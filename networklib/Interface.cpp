#include "pch.h"
#include "Interface.h"
#include "NetworkTransport.h"

void CreateNetworkTransport(INetworkTransport** ppTransport)
{
	if (ppTransport == nullptr)
		return;

	*ppTransport = new NetworkTransport();
}

void DeleteNetworkTransport(INetworkTransport* transport)
{
	delete transport;
}
