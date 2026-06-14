#pragma once

#ifdef NETWORKLIB_EXPORTS
#define NETWORKLIB_API __declspec(dllexport)
#else
#define NETWORKLIB_API __declspec(dllimport)
#endif

struct NetworkAddress
{
	unsigned int ipv4Address = 0;
	unsigned short port = 0;
};

struct INetworkTransport
{
	virtual ~INetworkTransport() = default;

	virtual bool StartHost(unsigned short port) = 0;
	virtual bool StartClient(const char* host, unsigned short port) = 0;
	virtual void Shutdown() = 0;
	virtual bool IsOpen() const = 0;
	virtual bool Send(const NetworkAddress& address, const void* packet, int packetSize) = 0;
	virtual bool SendToServer(const void* packet, int packetSize) = 0;
	virtual bool Receive(char* buffer, int bufferSize, int* bytesReceived, NetworkAddress* from) = 0;
	virtual void GetLocalAddressText(char* buffer, int bufferSize) const = 0;
};

extern "C" NETWORKLIB_API void CreateNetworkTransport(INetworkTransport** ppTransport);

extern "C" NETWORKLIB_API void DeleteNetworkTransport(INetworkTransport* transport);

extern "C" NETWORKLIB_API bool NetworkAddressesEqual(const NetworkAddress& left, const NetworkAddress& right);

extern "C" NETWORKLIB_API void GetPrimaryLocalNetworkAddress(char* buffer, int bufferSize);
