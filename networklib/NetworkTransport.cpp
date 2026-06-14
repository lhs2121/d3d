#include "pch.h"
#include "NetworkTransport.h"
#include <cstdio>
#include <cstring>

namespace
{
	NetworkAddress ToNetworkAddress(const sockaddr_in& address)
	{
		NetworkAddress result;
		result.ipv4Address = address.sin_addr.s_addr;
		result.port = address.sin_port;
		return result;
	}

	sockaddr_in ToSockAddr(const NetworkAddress& address)
	{
		sockaddr_in result = {};
		result.sin_family = AF_INET;
		result.sin_addr.s_addr = address.ipv4Address;
		result.sin_port = address.port;
		return result;
	}

	void WriteFallbackLocalAddress(char* buffer, int bufferSize)
	{
		if (buffer == nullptr || bufferSize <= 0)
			return;

		std::snprintf(buffer, static_cast<size_t>(bufferSize), "127.0.0.1");
	}

	void QueryPrimaryLocalAddress(char* buffer, int bufferSize)
	{
		WriteFallbackLocalAddress(buffer, bufferSize);
		if (buffer == nullptr || bufferSize <= 0)
			return;

		WSADATA wsaData = {};
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			return;

		char hostName[256] = {};
		if (gethostname(hostName, sizeof(hostName)) == 0)
		{
			addrinfo hints = {};
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_DGRAM;
			addrinfo* results = nullptr;
			if (getaddrinfo(hostName, nullptr, &hints, &results) == 0)
			{
				for (addrinfo* cursor = results; cursor != nullptr; cursor = cursor->ai_next)
				{
					if (cursor->ai_family != AF_INET || cursor->ai_addr == nullptr)
						continue;

					const sockaddr_in* address = reinterpret_cast<const sockaddr_in*>(cursor->ai_addr);
					char addressText[INET_ADDRSTRLEN] = {};
					const char* text = InetNtopA(AF_INET, const_cast<IN_ADDR*>(&address->sin_addr), addressText, sizeof(addressText));
					if (text == nullptr || addressText[0] == '\0')
						continue;
					if (std::strncmp(addressText, "127.", 4) == 0 || std::strncmp(addressText, "169.254.", 8) == 0)
						continue;

					std::snprintf(buffer, static_cast<size_t>(bufferSize), "%s", addressText);
					break;
				}
				freeaddrinfo(results);
			}
		}

		WSACleanup();
	}
}

NetworkTransport::~NetworkTransport()
{
	Shutdown();
}

bool NetworkTransport::EnsureWinsock()
{
	if (m_winsockStarted)
		return true;

	WSADATA wsaData = {};
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return false;

	m_winsockStarted = true;
	return true;
}

bool NetworkTransport::StartHost(unsigned short port)
{
	Shutdown();
	if (!EnsureWinsock())
		return false;

	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_socket == INVALID_SOCKET)
	{
		Shutdown();
		return false;
	}

	sockaddr_in bindAddress = {};
	bindAddress.sin_family = AF_INET;
	bindAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	bindAddress.sin_port = htons(port);
	if (bind(m_socket, reinterpret_cast<const sockaddr*>(&bindAddress), sizeof(bindAddress)) == SOCKET_ERROR)
	{
		Shutdown();
		return false;
	}

	u_long nonBlocking = 1;
	if (ioctlsocket(m_socket, FIONBIO, &nonBlocking) == SOCKET_ERROR)
	{
		Shutdown();
		return false;
	}

	return true;
}

bool NetworkTransport::StartClient(const char* host, unsigned short port)
{
	if (host == nullptr || host[0] == '\0')
		return false;

	Shutdown();
	if (!EnsureWinsock())
		return false;

	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_socket == INVALID_SOCKET)
	{
		Shutdown();
		return false;
	}

	sockaddr_in bindAddress = {};
	bindAddress.sin_family = AF_INET;
	bindAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	bindAddress.sin_port = 0;
	if (bind(m_socket, reinterpret_cast<const sockaddr*>(&bindAddress), sizeof(bindAddress)) == SOCKET_ERROR)
	{
		Shutdown();
		return false;
	}

	u_long nonBlocking = 1;
	if (ioctlsocket(m_socket, FIONBIO, &nonBlocking) == SOCKET_ERROR)
	{
		Shutdown();
		return false;
	}

	m_serverAddress = {};
	m_serverAddress.sin_family = AF_INET;
	m_serverAddress.sin_port = htons(port);
	if (InetPtonA(AF_INET, host, &m_serverAddress.sin_addr) != 1)
	{
		addrinfo hints = {};
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		addrinfo* result = nullptr;
		char portText[16] = {};
		std::snprintf(portText, sizeof(portText), "%hu", port);
		if (getaddrinfo(host, portText, &hints, &result) != 0 || result == nullptr)
		{
			Shutdown();
			return false;
		}

		m_serverAddress = *reinterpret_cast<sockaddr_in*>(result->ai_addr);
		freeaddrinfo(result);
	}

	return true;
}

void NetworkTransport::Shutdown()
{
	if (m_socket != INVALID_SOCKET)
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}

	m_serverAddress = {};

	if (m_winsockStarted)
	{
		WSACleanup();
		m_winsockStarted = false;
	}
}

bool NetworkTransport::IsOpen() const
{
	return m_socket != INVALID_SOCKET;
}

bool NetworkTransport::Send(const NetworkAddress& address, const void* packet, int packetSize)
{
	if (m_socket == INVALID_SOCKET || packet == nullptr || packetSize <= 0)
		return false;

	const sockaddr_in target = ToSockAddr(address);
	const int sent = sendto(
		m_socket,
		static_cast<const char*>(packet),
		packetSize,
		0,
		reinterpret_cast<const sockaddr*>(&target),
		sizeof(target));
	return sent == packetSize;
}

bool NetworkTransport::SendToServer(const void* packet, int packetSize)
{
	return Send(ToNetworkAddress(m_serverAddress), packet, packetSize);
}

bool NetworkTransport::Receive(char* buffer, int bufferSize, int* bytesReceived, NetworkAddress* from)
{
	if (bytesReceived != nullptr)
		*bytesReceived = 0;
	if (from != nullptr)
		*from = NetworkAddress();
	if (m_socket == INVALID_SOCKET || buffer == nullptr || bufferSize <= 0 || bytesReceived == nullptr || from == nullptr)
		return false;

	sockaddr_in sender = {};
	int senderLength = sizeof(sender);
	const int received = recvfrom(
		m_socket,
		buffer,
		bufferSize,
		0,
		reinterpret_cast<sockaddr*>(&sender),
		&senderLength);
	if (received == SOCKET_ERROR)
		return false;

	*bytesReceived = received;
	*from = ToNetworkAddress(sender);
	return true;
}

void NetworkTransport::GetLocalAddressText(char* buffer, int bufferSize) const
{
	QueryPrimaryLocalAddress(buffer, bufferSize);
}

bool NetworkAddressesEqual(const NetworkAddress& left, const NetworkAddress& right)
{
	return left.ipv4Address == right.ipv4Address && left.port == right.port;
}

void GetPrimaryLocalNetworkAddress(char* buffer, int bufferSize)
{
	QueryPrimaryLocalAddress(buffer, bufferSize);
}
