#pragma once
#include "Interface.h"
#include <winsock2.h>

class NetworkTransport final : public INetworkTransport
{
public:
	NetworkTransport() = default;
	~NetworkTransport();

	bool StartHost(unsigned short port) override;
	bool StartClient(const char* host, unsigned short port) override;
	void Shutdown() override;
	bool IsOpen() const override;
	bool Send(const NetworkAddress& address, const void* packet, int packetSize) override;
	bool SendToServer(const void* packet, int packetSize) override;
	bool Receive(char* buffer, int bufferSize, int* bytesReceived, NetworkAddress* from) override;
	void GetLocalAddressText(char* buffer, int bufferSize) const override;

private:
	bool EnsureWinsock();

	SOCKET m_socket = INVALID_SOCKET;
	sockaddr_in m_serverAddress = {};
	bool m_winsockStarted = false;
};
