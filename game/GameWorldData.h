#pragma once

#include <rendererlib/Interface.h>
#include <vector>
#include "GameStates.h"

class GameWorldData
{
public:
	void Clear()
	{
		blocks.clear();
		blockBreaks.clear();
		surfaceHeights.clear();
		biomes.clear();
		revealedTiles.clear();
	}

	std::vector<BlockTile> blocks;
	std::vector<BlockBreakState> blockBreaks;
	std::vector<int> surfaceHeights;
	std::vector<unsigned char> biomes;
	std::vector<unsigned char> revealedTiles;
};
