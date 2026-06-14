#pragma once

#include <vector>
#include "GameStates.h"

class GameNetworkState
{
public:
	void ClearRuntime()
	{
		peers.clear();
		remotePlayers.clear();
		remoteBlockBreaks.clear();
		tileHistory.clear();
		pendingTileEdits.clear();
		pendingDroppedItems.clear();
	}

	std::vector<RemotePlayerState> remotePlayers;
	std::vector<RemoteBlockBreakState> remoteBlockBreaks;
	std::vector<NetworkPeerState> peers;
	std::vector<NetworkTileEditState> tileHistory;
	std::vector<NetworkTileEditState> pendingTileEdits;
	std::vector<DroppedItemState> pendingDroppedItems;
};
