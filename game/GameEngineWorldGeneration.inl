// Included by GameEngine.cpp. Shares its private class declarations and file-local helpers.

void GameEngine::InitializeWorld()
{
	m_world.blocks.assign(m_blockWidth * m_blockHeight, BlockTile{});
	m_world.blockBreaks.assign(m_blockWidth * m_blockHeight, BlockBreakState{});
	m_world.surfaceHeights.clear();
	m_world.biomes.clear();
	m_world.revealedTiles.assign(static_cast<size_t>(m_blockWidth * m_blockHeight), 0);
	m_monsters.clear();
	m_droppedItems.clear();
	m_leafParticles.clear();
	m_projectiles.clear();
	m_networkState.remoteBlockBreaks.clear();
	m_monsterQueryScratch.clear();
	m_monsterOverlapScratch.clear();
	ResetMinimapTextureCache();
	m_debugRevealMap = false;
	m_minimapExpanded = false;
	m_networkBreakingBlockIndex = -1;
	m_worldOriginX = -((m_blockWidth - 1) * m_tileSize) * 0.5f;

	if (m_worldSeed == 0)
		m_worldSeed = GetTickCount();

	const unsigned int seed = m_worldSeed;
	std::mt19937 rng(seed);
	auto randomInt = [&rng](int minValue, int maxValue)
	{
		std::uniform_int_distribution<int> distribution(minValue, maxValue);
		return distribution(rng);
	};
	auto randomFloat = [&rng](float minValue, float maxValue)
	{
		std::uniform_real_distribution<float> distribution(minValue, maxValue);
		return distribution(rng);
	};

	std::vector<int> surfaceHeights(m_blockWidth, m_surfaceRow);
	std::vector<BiomeType> biomes(m_blockWidth, BiomeGrassland);
	const int spawnX = m_blockWidth / 5;
	const int spawnSafeRadius = 10;
	const int grasslandEnd = (m_blockWidth * 43) / 100;
	const int desertEnd = (m_blockWidth * 70) / 100;

	for (int x = 0; x < m_blockWidth; ++x)
	{
		const int biomeWarp = static_cast<int>((FractalNoise1D(x * 0.024f + 71.0f, seed + 17u, 4, 2.0f, 0.55f) - 0.5f) * 30.0f);
		const int biomeX = x + biomeWarp;
		if (std::abs(x - spawnX) <= spawnSafeRadius + 6)
		{
			biomes[x] = BiomeGrassland;
		}
		else if (biomeX < grasslandEnd)
		{
			biomes[x] = BiomeGrassland;
		}
		else if (biomeX < desertEnd)
		{
			biomes[x] = BiomeDesert;
		}
		else
		{
			biomes[x] = BiomeIce;
		}

		const float continent = FractalNoise1D(x * 0.010f, seed + 101u, 5, 2.0f, 0.55f) - 0.5f;
		const float hills = FractalNoise1D(x * 0.038f + 17.0f, seed + 211u, 4, 2.1f, 0.52f) - 0.5f;
		const float detail = FractalNoise1D(x * 0.110f + 41.0f, seed + 307u, 3, 2.2f, 0.48f) - 0.5f;

		float surface = static_cast<float>(m_surfaceRow) + continent * 8.0f + hills * 5.5f + detail * 2.0f;
		switch (biomes[x])
		{
		case BiomeDesert:
			surface = static_cast<float>(m_surfaceRow + 4) +
				(FractalNoise1D(x * 0.021f + 51.0f, seed + 401u, 3, 2.0f, 0.50f) - 0.5f) * 4.0f +
				std::sin(x * 0.13f) * 1.6f;
			break;
		case BiomeIce:
			surface = static_cast<float>(m_surfaceRow - 1) +
				continent * 7.0f + hills * 7.5f + std::fabs(detail) * 3.0f -
				(FractalNoise1D(x * 0.017f + 181.0f, seed + 417u, 4, 2.0f, 0.52f) - 0.5f) * 3.0f;
			break;
		case BiomeGrassland:
		default:
			break;
		}

		surfaceHeights[x] = std::clamp(static_cast<int>(std::round(surface)), 8, m_blockHeight / 2);
	}

	for (int pass = 0; pass < 3; ++pass)
	{
		std::vector<int> smoothedHeights = surfaceHeights;
		for (int x = 1; x < m_blockWidth - 1; ++x)
			smoothedHeights[x] = (surfaceHeights[x - 1] + surfaceHeights[x] * 2 + surfaceHeights[x + 1]) / 4;

		surfaceHeights.swap(smoothedHeights);
	}

	const int spawnSurfaceY = std::clamp(surfaceHeights[spawnX], m_surfaceRow - 2, m_surfaceRow + 2);
	for (int x = spawnX - spawnSafeRadius; x <= spawnX + spawnSafeRadius; ++x)
	{
		if (x < 0 || x >= m_blockWidth)
			continue;

		const int distance = std::abs(x - spawnX);
		surfaceHeights[x] = spawnSurfaceY + (distance > 6 ? (distance - 6) / 4 : 0);
		biomes[x] = BiomeGrassland;
	}
	m_world.surfaceHeights = surfaceHeights;
	m_world.biomes.resize(m_blockWidth);
	for (int x = 0; x < m_blockWidth; ++x)
		m_world.biomes[x] = static_cast<unsigned char>(biomes[x]);

	for (int y = 0; y < m_blockHeight; ++y)
	{
		for (int x = 0; x < m_blockWidth; ++x)
		{
			BlockTile& tile = m_world.blocks[y * m_blockWidth + x];
			const int surfaceY = surfaceHeights[x];
			const BiomeType biome = biomes[x];

			if (y < surfaceY)
			{
				tile.visible = 0;
				continue;
			}

			tile.visible = 1;
			const int depth = y - surfaceY;
			const float layerNoise = FractalNoise2D(x * 0.11f, y * 0.11f, seed + 503u, 3, 2.0f, 0.55f);
			const int soilDepth = 3 + static_cast<int>(FractalNoise1D(x * 0.045f, seed + 521u, 3, 2.0f, 0.50f) * 4.0f);
			tile.tileIndex = GetTerrainTileForBiome(biome, depth <= soilDepth ? depth : depth + 2, layerNoise);
		}
	}

	std::vector<unsigned char> caveMask(m_blockWidth * m_blockHeight, 0);
	auto markCaveCircle = [this, &surfaceHeights, &caveMask, spawnX, spawnSurfaceY, spawnSafeRadius](float centerX, float centerY, float radiusX, float radiusY)
	{
		const int minX = (std::max)(1, static_cast<int>(std::floor(centerX - radiusX - 1.0f)));
		const int maxX = (std::min)(m_blockWidth - 2, static_cast<int>(std::ceil(centerX + radiusX + 1.0f)));
		const int minY = (std::max)(1, static_cast<int>(std::floor(centerY - radiusY - 1.0f)));
		const int maxY = (std::min)(m_blockHeight - 2, static_cast<int>(std::ceil(centerY + radiusY + 1.0f)));

		for (int y = minY; y <= maxY; ++y)
		{
			for (int x = minX; x <= maxX; ++x)
			{
				if (y <= surfaceHeights[x] + 8)
					continue;
				if (std::abs(x - spawnX) < spawnSafeRadius + 5 && y < spawnSurfaceY + 15)
					continue;

				const float dx = (static_cast<float>(x) - centerX) / radiusX;
				const float dy = (static_cast<float>(y) - centerY) / radiusY;
				if (dx * dx + dy * dy <= 1.0f)
					caveMask[y * m_blockWidth + x] = 1;
			}
		}
	};

	struct CaveNode
	{
		float x;
		float y;
		float radius;
	};

	auto carveCaveTunnel = [this, &surfaceHeights, &randomFloat, &markCaveCircle](float startX, float startY, float endX, float endY, float baseRadius)
	{
		const float dx = endX - startX;
		const float dy = endY - startY;
		const float length = std::sqrt(dx * dx + dy * dy);
		const int steps = (std::max)(5, static_cast<int>(std::ceil(length * 0.75f)));
		const float wavePhase = randomFloat(0.0f, 6.2831853f);
		const float waveFrequency = randomFloat(0.9f, 1.9f);
		const float waveAmplitude = randomFloat(0.35f, 1.25f);

		for (int step = 0; step <= steps; ++step)
		{
			const float t = static_cast<float>(step) / static_cast<float>(steps);
			const float eased = SmoothStep(t);
			float caveX = Lerp(startX, endX, eased);
			float caveY = Lerp(startY, endY, eased);
			caveX += std::sin(t * 7.1f + wavePhase * 0.73f) * 0.9f;
			caveY += std::sin(t * 6.2831853f * waveFrequency + wavePhase) * waveAmplitude;

			const int tileX = std::clamp(static_cast<int>(std::round(caveX)), 2, m_blockWidth - 3);
			const float minY = static_cast<float>(surfaceHeights[tileX] + 10);
			caveY = std::clamp(caveY, minY, static_cast<float>(m_blockHeight - 4));

			const float depthRatio = static_cast<float>(caveY - surfaceHeights[tileX]) /
				static_cast<float>((std::max)(1, m_blockHeight - surfaceHeights[tileX]));
			const float radiusX = baseRadius + depthRatio * 0.75f + randomFloat(-0.10f, 0.16f);
			const float radiusY = baseRadius * 0.58f + depthRatio * 0.45f + randomFloat(-0.08f, 0.12f);
			markCaveCircle(caveX, caveY, (std::max)(0.70f, radiusX), (std::max)(0.48f, radiusY));
		}
	};

	for (int y = 1; y < m_blockHeight - 2; ++y)
	{
		for (int x = 1; x < m_blockWidth - 1; ++x)
		{
			if (y <= surfaceHeights[x] + 12)
				continue;
			if (std::abs(x - spawnX) < spawnSafeRadius + 6 && y < spawnSurfaceY + 18)
				continue;

			const float depthRatio = static_cast<float>(y - surfaceHeights[x]) / static_cast<float>(m_blockHeight - surfaceHeights[x]);
			const float caveNoise = FractalNoise2D(x * 0.067f, y * 0.083f, seed + 701u, 4, 2.0f, 0.54f);
			const float broadCavern = FractalNoise2D(x * 0.024f + 73.0f, y * 0.031f, seed + 705u, 3, 2.0f, 0.50f);
			const float wormNoise = std::fabs(FractalNoise2D(x * 0.030f + 19.0f, y * 0.030f, seed + 709u, 3, 2.0f, 0.50f) - 0.5f);
			const float threshold = 0.90f - depthRatio * 0.035f;
			if ((caveNoise > threshold && broadCavern > 0.74f && depthRatio > 0.40f) ||
				(wormNoise < 0.012f && depthRatio > 0.46f))
				caveMask[y * m_blockWidth + x] = 1;
		}
	}

	for (int pass = 0; pass < 2; ++pass)
	{
		std::vector<unsigned char> nextMask = caveMask;
		for (int y = 2; y < m_blockHeight - 2; ++y)
		{
			for (int x = 2; x < m_blockWidth - 2; ++x)
			{
				if (y <= surfaceHeights[x] + 10)
					continue;

				int caveNeighbors = 0;
				for (int oy = -1; oy <= 1; ++oy)
				{
					for (int ox = -1; ox <= 1; ++ox)
					{
						if (ox == 0 && oy == 0)
							continue;
						caveNeighbors += caveMask[(y + oy) * m_blockWidth + (x + ox)] != 0 ? 1 : 0;
					}
				}

				const int blockIndex = y * m_blockWidth + x;
				if (caveMask[blockIndex] != 0)
					nextMask[blockIndex] = caveNeighbors >= 3 ? 1 : 0;
				else if (caveNeighbors >= 7 && y > surfaceHeights[x] + 18)
					nextMask[y * m_blockWidth + x] = 1;
			}
		}

		caveMask.swap(nextMask);
	}

	const int tunnelCount = 2 + m_blockWidth / 240;
	for (int tunnel = 0; tunnel < tunnelCount; ++tunnel)
	{
		float caveX = static_cast<float>(randomInt(8, m_blockWidth - 9));
		float caveY = static_cast<float>(surfaceHeights[static_cast<int>(caveX)] + randomInt(14, m_blockHeight - surfaceHeights[static_cast<int>(caveX)] - 7));
		float angle = randomFloat(-1.8f, 1.8f);
		const int steps = randomInt(12, 32);

		for (int step = 0; step < steps; ++step)
		{
			const float depthRatio = caveY / static_cast<float>(m_blockHeight);
			markCaveCircle(caveX, caveY, randomFloat(0.60f, 1.05f + depthRatio * 0.55f), randomFloat(0.45f, 0.85f + depthRatio * 0.38f));
			angle += randomFloat(-0.26f, 0.26f);
			caveX += std::cos(angle) * randomFloat(0.65f, 1.45f);
			caveY += std::sin(angle) * randomFloat(0.35f, 1.00f) + randomFloat(-0.22f, 0.30f);
			caveX = std::clamp(caveX, 3.0f, static_cast<float>(m_blockWidth - 4));
			caveY = std::clamp(caveY, static_cast<float>(surfaceHeights[static_cast<int>(caveX)] + 11), static_cast<float>(m_blockHeight - 5));
		}
	}

	const int chamberCount = 2 + m_blockWidth / 240;
	std::vector<CaveNode> chamberNodes;
	chamberNodes.reserve(chamberCount);
	for (int chamber = 0; chamber < chamberCount; ++chamber)
	{
		const int centerX = randomInt(8, m_blockWidth - 9);
		const int minY = surfaceHeights[centerX] + 18;
		if (minY >= m_blockHeight - 10)
			continue;

		const int centerY = randomInt(minY, m_blockHeight - 9);
		const float depthRatio = static_cast<float>(centerY) / static_cast<float>(m_blockHeight);
		const float radiusX = randomFloat(1.9f, 3.2f + depthRatio * 1.2f);
		const float radiusY = randomFloat(1.00f, 1.85f + depthRatio * 0.75f);
		markCaveCircle(static_cast<float>(centerX), static_cast<float>(centerY), radiusX, radiusY);
		chamberNodes.push_back({ static_cast<float>(centerX), static_cast<float>(centerY), (radiusX + radiusY) * 0.5f });
	}

	std::sort(chamberNodes.begin(), chamberNodes.end(), [](const CaveNode& lhs, const CaveNode& rhs)
	{
		return lhs.x < rhs.x;
	});

	for (size_t i = 1; i < chamberNodes.size(); ++i)
	{
		if (randomInt(0, 99) > 38)
			continue;

		const CaveNode& previous = chamberNodes[i - 1];
		const CaveNode& current = chamberNodes[i];
		if (current.x - previous.x > 42.0f)
			continue;

		const float connectorRadius = std::clamp((previous.radius + current.radius) * 0.11f, 0.75f, 1.35f);
		carveCaveTunnel(previous.x, previous.y, current.x, current.y, connectorRadius);
	}

	const int extraConnectorCount = (std::min)(2, static_cast<int>(chamberNodes.size()));
	for (int connector = 0; connector < extraConnectorCount && chamberNodes.size() > 2; ++connector)
	{
		const int fromIndex = randomInt(0, static_cast<int>(chamberNodes.size()) - 2);
		const int toIndex = std::clamp(fromIndex + randomInt(1, 2), 0, static_cast<int>(chamberNodes.size()) - 1);
		const CaveNode& from = chamberNodes[fromIndex];
		const CaveNode& to = chamberNodes[toIndex];
		if (std::fabs(to.x - from.x) > 46.0f)
			continue;

		const float connectorRadius = std::clamp((from.radius + to.radius) * 0.10f, 0.70f, 1.25f);
		carveCaveTunnel(from.x, from.y, to.x, to.y, connectorRadius);
	}

	for (int y = 0; y < m_blockHeight; ++y)
	{
		for (int x = 0; x < m_blockWidth; ++x)
		{
			if (caveMask[y * m_blockWidth + x] != 0)
				m_world.blocks[y * m_blockWidth + x].visible = 0;
		}
	}

	auto restoreTerrainTile = [this, &surfaceHeights, &biomes, seed](int tileX, int tileY)
	{
		if (!IsTileInBounds(tileX, tileY) || tileY < surfaceHeights[tileX])
			return;

		BlockTile& tile = m_world.blocks[tileY * m_blockWidth + tileX];
		const int depth = tileY - surfaceHeights[tileX];
		const BiomeType biome = biomes[tileX];
		const float layerNoise = FractalNoise2D(tileX * 0.11f, tileY * 0.11f, seed + 503u, 3, 2.0f, 0.55f);
		tile.visible = 1;
		tile.tileIndex = GetTerrainTileForBiome(biome, depth, layerNoise);
	};

	auto reinforceTerrainMass = [this, &surfaceHeights, seed, &restoreTerrainTile]()
	{
		for (int x = 0; x < m_blockWidth; ++x)
		{
			const int crustBottom = (std::min)(surfaceHeights[x] + 9, m_blockHeight - 1);
			for (int y = surfaceHeights[x]; y <= crustBottom; ++y)
				restoreTerrainTile(x, y);
		}

		for (int y = (std::max)(0, m_blockHeight - 5); y < m_blockHeight; ++y)
		{
			for (int x = 0; x < m_blockWidth; ++x)
				restoreTerrainTile(x, y);
		}

		for (int pass = 0; pass < 2; ++pass)
		{
			std::vector<unsigned char> refill(m_blockWidth * m_blockHeight, 0);
			for (int y = 1; y < m_blockHeight - 1; ++y)
			{
				for (int x = 1; x < m_blockWidth - 1; ++x)
				{
					const int blockIndex = y * m_blockWidth + x;
					if (m_world.blocks[blockIndex].visible != 0 || y <= surfaceHeights[x] + 10)
						continue;

					int solidNeighbors = 0;
					for (int oy = -1; oy <= 1; ++oy)
					{
						for (int ox = -1; ox <= 1; ++ox)
						{
							if (ox == 0 && oy == 0)
								continue;

							const int neighborIndex = (y + oy) * m_blockWidth + (x + ox);
							solidNeighbors += m_world.blocks[neighborIndex].visible != 0 ? 1 : 0;
						}
					}

					const float depthRatio = static_cast<float>(y - surfaceHeights[x]) /
						static_cast<float>((std::max)(1, m_blockHeight - surfaceHeights[x]));
					const float localNoise = Hash01(x, y, seed + 1301u);
					if (solidNeighbors >= 6 || (solidNeighbors >= 5 && depthRatio > 0.42f && localNoise > 0.35f))
						refill[blockIndex] = 1;
				}
			}

			for (int y = 1; y < m_blockHeight - 1; ++y)
			{
				for (int x = 1; x < m_blockWidth - 1; ++x)
				{
					if (refill[y * m_blockWidth + x] != 0)
						restoreTerrainTile(x, y);
				}
			}
		}
	};

	reinforceTerrainMass();

	auto carveSolidCircle = [this, &surfaceHeights, spawnX, spawnSurfaceY, spawnSafeRadius](float centerX, float centerY, float radiusX, float radiusY, bool allowSurfaceOpening)
	{
		const int minX = (std::max)(1, static_cast<int>(std::floor(centerX - radiusX - 1.0f)));
		const int maxX = (std::min)(m_blockWidth - 2, static_cast<int>(std::ceil(centerX + radiusX + 1.0f)));
		const int minY = (std::max)(1, static_cast<int>(std::floor(centerY - radiusY - 1.0f)));
		const int maxY = (std::min)(m_blockHeight - 2, static_cast<int>(std::ceil(centerY + radiusY + 1.0f)));

		for (int y = minY; y <= maxY; ++y)
		{
			for (int x = minX; x <= maxX; ++x)
			{
				if (y < surfaceHeights[x])
					continue;
				if (!allowSurfaceOpening && y <= surfaceHeights[x] + 9)
					continue;
				if (std::abs(x - spawnX) < spawnSafeRadius + 6 && y < spawnSurfaceY + 18)
					continue;

				const float dx = (static_cast<float>(x) - centerX) / radiusX;
				const float dy = (static_cast<float>(y) - centerY) / radiusY;
				if (dx * dx + dy * dy <= 1.0f)
					m_world.blocks[y * m_blockWidth + x].visible = 0;
			}
		}
	};

	auto carveSolidTunnel = [this, &surfaceHeights, &randomFloat, &carveSolidCircle](float startX, float startY, float endX, float endY, float baseRadius, bool allowSurfaceOpening)
	{
		const float dx = endX - startX;
		const float dy = endY - startY;
		const float length = std::sqrt(dx * dx + dy * dy);
		const int steps = (std::max)(8, static_cast<int>(std::ceil(length * 1.15f)));
		const float wavePhase = randomFloat(0.0f, 6.2831853f);
		const float waveFrequency = randomFloat(0.85f, 1.70f);
		const float waveAmplitude = randomFloat(0.45f, allowSurfaceOpening ? 1.10f : 1.75f);

		for (int step = 0; step <= steps; ++step)
		{
			const float t = static_cast<float>(step) / static_cast<float>(steps);
			const float eased = SmoothStep(t);
			float caveX = Lerp(startX, endX, eased);
			float caveY = Lerp(startY, endY, eased);
			caveX += std::sin(t * 7.6f + wavePhase * 0.53f) * 0.95f;
			caveY += std::sin(t * 6.2831853f * waveFrequency + wavePhase) * waveAmplitude;

			const int tileX = std::clamp(static_cast<int>(std::round(caveX)), 2, m_blockWidth - 3);
			const float minimumY = static_cast<float>(surfaceHeights[tileX] + (allowSurfaceOpening ? 1 : 11));
			caveY = std::clamp(caveY, minimumY, static_cast<float>(m_blockHeight - 6));

			const float depthRatio = static_cast<float>(caveY - surfaceHeights[tileX]) /
				static_cast<float>((std::max)(1, m_blockHeight - surfaceHeights[tileX]));
			const float radiusX = baseRadius + depthRatio * 0.48f + randomFloat(-0.12f, 0.16f);
			const float radiusY = baseRadius * 0.84f + depthRatio * 0.30f + randomFloat(-0.08f, 0.12f);
			carveSolidCircle(caveX, caveY, (std::max)(0.95f, radiusX), (std::max)(1.08f, radiusY), allowSurfaceOpening);
		}
	};

	struct CaveBranchJob
	{
		CaveNode from;
		float angle;
		int depth;
		int branchIndex;
		bool forceLobby;
	};

	auto clampCavePoint = [this, &surfaceHeights](float& caveX, float& caveY, int minDepth)
	{
		caveX = std::clamp(caveX, 6.0f, static_cast<float>(m_blockWidth - 7));
		const int tileX = std::clamp(static_cast<int>(std::round(caveX)), 2, m_blockWidth - 3);
		const float minimumY = static_cast<float>((std::min)(surfaceHeights[tileX] + minDepth, m_blockHeight - 8));
		caveY = std::clamp(caveY, minimumY, static_cast<float>(m_blockHeight - 8));
	};

	auto carveLobedChamber = [&randomInt, &randomFloat, &carveSolidCircle](float centerX, float centerY,
		float radiusX, float radiusY, bool allowSurfaceOpening)
	{
		carveSolidCircle(centerX, centerY, radiusX, radiusY, allowSurfaceOpening);
		const int lobeCount = randomInt(0, 1);
		for (int lobe = 0; lobe < lobeCount; ++lobe)
		{
			const float angle = randomFloat(-3.1415926f, 3.1415926f);
			const float offsetX = std::cos(angle) * radiusX * randomFloat(0.22f, 0.42f);
			const float offsetY = std::sin(angle) * radiusY * randomFloat(0.14f, 0.30f);
			carveSolidCircle(centerX + offsetX, centerY + offsetY,
				radiusX * randomFloat(0.36f, 0.56f),
				radiusY * randomFloat(0.38f, 0.62f),
				allowSurfaceOpening);
		}

		return CaveNode{ centerX, centerY, (radiusX + radiusY) * 0.5f };
	};

	auto carveNaturalPath = [&randomFloat, &carveSolidTunnel](const CaveNode& from, float endX, float endY,
		float radius, bool allowSurfaceOpening)
	{
		const float midX = Lerp(from.x, endX, randomFloat(0.38f, 0.62f));
		const float midY = Lerp(from.y, endY, randomFloat(0.38f, 0.62f));
		const float dx = endX - from.x;
		const float dy = endY - from.y;
		const float length = std::sqrt(dx * dx + dy * dy);
		const float normalX = length > 0.01f ? -dy / length : 0.0f;
		const float normalY = length > 0.01f ? dx / length : 0.0f;
		const float bend = randomFloat(-7.5f, 7.5f);
		const float jointX = midX + normalX * bend;
		const float jointY = midY + normalY * bend * 0.55f;

		carveSolidTunnel(from.x, from.y, jointX, jointY, radius * randomFloat(0.88f, 1.08f), allowSurfaceOpening);
		carveSolidTunnel(jointX, jointY, endX, endY, radius * randomFloat(0.82f, 1.04f), allowSurfaceOpening);
	};

	std::vector<CaveNode> caveGraphNodes;
	std::vector<CaveBranchJob> branchJobs;
	std::vector<CaveNode> mainRouteNodes;
	caveGraphNodes.reserve(200);
	branchJobs.reserve(160);
	mainRouteNodes.reserve(16);

	struct EntrancePlan
	{
		int minX;
		int maxX;
		int entranceCount;
		int primaryBranchCount;
	};

	const EntrancePlan entrancePlans[] =
	{
		{
			spawnX + spawnSafeRadius + 34,
			grasslandEnd - 20,
			2,
			4
		},
		{
			grasslandEnd + 22,
			desertEnd - 22,
			2,
			4
		},
		{
			desertEnd + 22,
			m_blockWidth - 18,
			2,
			4
		},
	};

	for (int entranceIndex = 0; entranceIndex < 3; ++entranceIndex)
	{
		const EntrancePlan& plan = entrancePlans[entranceIndex];
		if (plan.minX > plan.maxX)
			continue;

		const int usableWidth = plan.maxX - plan.minX + 1;
		const int entranceCount = std::clamp(plan.entranceCount, 1, (std::max)(1, usableWidth / 34));
		for (int entranceSlot = 0; entranceSlot < entranceCount; ++entranceSlot)
		{
			const float slotT = static_cast<float>(entranceSlot + 1) / static_cast<float>(entranceCount + 1);
			int entranceX = static_cast<int>(std::round(Lerp(static_cast<float>(plan.minX), static_cast<float>(plan.maxX), slotT)));
			entranceX = std::clamp(entranceX + randomInt(-9, 9), plan.minX, plan.maxX);
			if (std::abs(entranceX - spawnX) < spawnSafeRadius + 22)
				entranceX = std::clamp(spawnX + spawnSafeRadius + 34, plan.minX, plan.maxX);

			const int entranceSurfaceY = surfaceHeights[entranceX];
			const int mouthHalfWidth = randomInt(1, 2);
			for (int dx = -mouthHalfWidth; dx <= mouthHalfWidth; ++dx)
			{
				const int tileX = entranceX + dx;
				if (tileX < 1 || tileX >= m_blockWidth - 1)
					continue;

				const int openDepth = mouthHalfWidth + 4 - std::abs(dx);
				for (int y = (std::max)(0, surfaceHeights[tileX] - 1);
					y <= surfaceHeights[tileX] + openDepth && y < m_blockHeight - 1; ++y)
				{
					m_world.blocks[y * m_blockWidth + tileX].visible = 0;
				}
			}

			carveSolidCircle(static_cast<float>(entranceX), static_cast<float>(entranceSurfaceY + 3),
				static_cast<float>(mouthHalfWidth) + 0.65f, randomFloat(2.0f, 2.8f), true);

			float lobbyX = static_cast<float>(entranceX + randomInt(-5, 5));
			float lobbyY = static_cast<float>(entranceSurfaceY + randomInt(14, 21));
			clampCavePoint(lobbyX, lobbyY, 13);
			CaveNode rootLobby = carveLobedChamber(lobbyX, lobbyY, randomFloat(2.0f, 3.1f), randomFloat(0.95f, 1.55f), false);
			CaveNode entranceNode{ static_cast<float>(entranceX), static_cast<float>(entranceSurfaceY + 3), 1.5f };
			carveNaturalPath(entranceNode, rootLobby.x, rootLobby.y, randomFloat(0.92f, 1.25f), true);

			float routeX = rootLobby.x + randomFloat(-7.0f, 7.0f);
			float routeY = rootLobby.y + randomFloat(12.0f, 20.0f);
			clampCavePoint(routeX, routeY, 24);
			CaveNode routeHub = carveLobedChamber(routeX, routeY, randomFloat(1.25f, 2.05f), randomFloat(0.70f, 1.25f), false);
			carveNaturalPath(rootLobby, routeHub.x, routeHub.y, randomFloat(0.64f, 0.92f), false);

			caveGraphNodes.push_back(rootLobby);
			caveGraphNodes.push_back(routeHub);
			mainRouteNodes.push_back(routeHub);
			const int branchCount = entranceSlot == 0 ? plan.primaryBranchCount : 3;
			for (int branch = 0; branch < branchCount; ++branch)
			{
				const float branchT = branchCount <= 1 ? 0.5f : static_cast<float>(branch) / static_cast<float>(branchCount - 1);
				const float angle = Lerp(0.46f, 2.70f, branchT) + randomFloat(-0.18f, 0.18f);
				branchJobs.push_back({
					rootLobby,
					angle,
					0,
					branch,
					entranceSlot == 0 && branch == 0
				});
			}

			const int routeBranchCount = entranceSlot == 0 ? 2 : 1;
			for (int branch = 0; branch < routeBranchCount; ++branch)
			{
				const float sideSign = branch == 0 ? -1.0f : 1.0f;
				float angle = randomFloat(0.72f, 2.42f) + sideSign * randomFloat(0.20f, 0.52f);
				angle = std::clamp(angle, 0.22f, 2.92f);
				branchJobs.push_back({ routeHub, angle, 1, branch, false });
			}
		}
	}

	std::sort(mainRouteNodes.begin(), mainRouteNodes.end(), [](const CaveNode& lhs, const CaveNode& rhs)
	{
		return lhs.x < rhs.x;
	});

	for (size_t routeIndex = 1; routeIndex < mainRouteNodes.size(); ++routeIndex)
	{
		const CaveNode& previous = mainRouteNodes[routeIndex - 1];
		const CaveNode& next = mainRouteNodes[routeIndex];
		const float dx = next.x - previous.x;
		const float dy = next.y - previous.y;
		const float distance = std::sqrt(dx * dx + dy * dy);
		const int relayCount = std::clamp(static_cast<int>(std::ceil(distance / 62.0f)), 1, 4);
		CaveNode cursor = previous;

		for (int relay = 1; relay <= relayCount; ++relay)
		{
			const float t = static_cast<float>(relay) / static_cast<float>(relayCount + 1);
			float relayX = Lerp(previous.x, next.x, t) + randomFloat(-9.0f, 9.0f);
			float relayY = Lerp(previous.y, next.y, t) + randomFloat(-8.0f, 11.0f);
			clampCavePoint(relayX, relayY, 26);

			const float connectorRadius = randomFloat(0.70f, 0.98f);
			carveNaturalPath(cursor, relayX, relayY, connectorRadius, false);
			CaveNode relayNode = carveLobedChamber(relayX, relayY, randomFloat(1.15f, 2.35f), randomFloat(0.72f, 1.45f), false);
			caveGraphNodes.push_back(relayNode);

			if (branchJobs.size() < 160 && randomInt(0, 99) < 76)
			{
				const float sideSign = randomInt(0, 1) == 0 ? -1.0f : 1.0f;
				const float baseAngle = std::atan2(next.y - previous.y, next.x - previous.x);
				float branchAngle = baseAngle + sideSign * randomFloat(0.62f, 1.28f) + randomFloat(-0.18f, 0.18f);
				branchAngle = std::clamp(branchAngle, 0.18f, 2.96f);
				branchJobs.push_back({ relayNode, branchAngle, 1, relay, false });
			}

			cursor = relayNode;
		}

		carveNaturalPath(cursor, next.x, next.y, randomFloat(0.70f, 1.02f), false);
	}

	std::vector<CaveNode> ambientCaveNodes;
	const int ambientNodeCount = (std::max)(8, m_blockWidth / 56);
	ambientCaveNodes.reserve(ambientNodeCount);
	for (int ambient = 0; ambient < ambientNodeCount; ++ambient)
	{
		const float t = static_cast<float>(ambient + 1) / static_cast<float>(ambientNodeCount + 1);
		float ambientX = Lerp(18.0f, static_cast<float>(m_blockWidth - 19), t) + randomFloat(-14.0f, 14.0f);
		ambientX = std::clamp(ambientX, 8.0f, static_cast<float>(m_blockWidth - 9));

		const int tileX = std::clamp(static_cast<int>(std::round(ambientX)), 2, m_blockWidth - 3);
		const int minY = (std::min)(surfaceHeights[tileX] + randomInt(24, 38), m_blockHeight - 10);
		const int maxY = (std::min)(surfaceHeights[tileX] + randomInt(56, 86), m_blockHeight - 8);
		if (minY >= maxY)
			continue;

		float ambientY = static_cast<float>(randomInt(minY, maxY));
		clampCavePoint(ambientX, ambientY, 22);
		CaveNode ambientNode = carveLobedChamber(ambientX, ambientY,
			randomFloat(1.35f, 2.65f), randomFloat(0.78f, 1.55f), false);
		ambientCaveNodes.push_back(ambientNode);
		caveGraphNodes.push_back(ambientNode);

		const int branchCount = randomInt(1, 2);
		for (int branch = 0; branch < branchCount && branchJobs.size() < 160; ++branch)
		{
			const float branchT = branchCount <= 1 ? 0.5f : static_cast<float>(branch) / static_cast<float>(branchCount - 1);
			const float angle = Lerp(0.44f, 2.70f, branchT) + randomFloat(-0.38f, 0.38f);
			branchJobs.push_back({ ambientNode, std::clamp(angle, 0.18f, 2.98f), 1, branch, false });
		}
	}

	std::sort(ambientCaveNodes.begin(), ambientCaveNodes.end(), [](const CaveNode& lhs, const CaveNode& rhs)
	{
		return lhs.x < rhs.x;
	});

	for (size_t i = 1; i < ambientCaveNodes.size(); ++i)
	{
		const CaveNode& previous = ambientCaveNodes[i - 1];
		const CaveNode& current = ambientCaveNodes[i];
		if (current.x - previous.x > 92.0f || std::fabs(current.y - previous.y) > 48.0f)
			continue;

		carveNaturalPath(previous, current.x, current.y, randomFloat(0.52f, 0.78f), false);
	}

	for (const CaveNode& ambientNode : ambientCaveNodes)
	{
		if (mainRouteNodes.empty())
			break;

		const CaveNode* nearestRoute = nullptr;
		float nearestDistanceSq = 999999.0f;
		for (const CaveNode& routeNode : mainRouteNodes)
		{
			const float dx = routeNode.x - ambientNode.x;
			const float dy = routeNode.y - ambientNode.y;
			const float distanceSq = dx * dx + dy * dy;
			if (distanceSq < nearestDistanceSq)
			{
				nearestDistanceSq = distanceSq;
				nearestRoute = &routeNode;
			}
		}

		if (nearestRoute == nullptr || nearestDistanceSq > 132.0f * 132.0f)
			continue;
		if (randomInt(0, 99) > 58 && nearestDistanceSq > 72.0f * 72.0f)
			continue;

		carveNaturalPath(ambientNode, nearestRoute->x, nearestRoute->y, randomFloat(0.50f, 0.74f), false);
	}

	for (size_t jobIndex = 0; jobIndex < branchJobs.size() && jobIndex < 132; ++jobIndex)
	{
		const CaveBranchJob job = branchJobs[jobIndex];
		const float length = job.depth == 0 ? randomFloat(24.0f, 42.0f) :
			(job.depth == 1 ? randomFloat(17.0f, 31.0f) :
				(job.depth == 2 ? randomFloat(11.0f, 22.0f) : randomFloat(7.0f, 15.0f)));
		float endX = job.from.x + std::cos(job.angle) * length + randomFloat(-6.0f, 6.0f);
		float endY = job.from.y + std::sin(job.angle) * length + randomFloat(-3.0f, 8.0f);
		const int minDepth = 16 + job.depth * 4;
		clampCavePoint(endX, endY, minDepth);

		const float tunnelRadius = job.depth == 0 ? randomFloat(1.02f, 1.42f) :
			(job.depth == 1 ? randomFloat(0.88f, 1.18f) :
				(job.depth == 2 ? randomFloat(0.72f, 1.00f) : randomFloat(0.62f, 0.88f)));
		carveNaturalPath(job.from, endX, endY, tunnelRadius, false);

		const int pocketCount = job.depth <= 1 ? randomInt(1, 2) : (randomInt(0, 99) < 42 ? 1 : 0);
		for (int pocket = 0; pocket < pocketCount; ++pocket)
		{
			const float pocketT = randomFloat(0.26f, 0.86f);
			float pocketX = Lerp(job.from.x, endX, pocketT);
			float pocketY = Lerp(job.from.y, endY, pocketT);
			const float sideSign = randomInt(0, 1) == 0 ? -1.0f : 1.0f;
			const float offset = randomFloat(1.8f, 4.2f);
			pocketX += std::cos(job.angle + sideSign * 1.5707963f) * offset;
			pocketY += std::sin(job.angle + sideSign * 1.5707963f) * offset * 0.62f;
			clampCavePoint(pocketX, pocketY, minDepth);
			carveLobedChamber(pocketX, pocketY, randomFloat(1.0f, 2.1f), randomFloat(0.70f, 1.45f), false);
		}

		if (job.depth < 3 && branchJobs.size() < 160)
		{
			int twigCount = 0;
			if (job.depth == 0)
				twigCount = randomInt(1, 2);
			else if (job.depth == 1)
				twigCount = randomInt(0, 99) < 72 ? 1 : 0;
			else
				twigCount = randomInt(0, 99) < 34 ? 1 : 0;

			for (int twig = 0; twig < twigCount && branchJobs.size() < 160; ++twig)
			{
				const float twigT = randomFloat(0.34f, 0.80f);
				float twigX = Lerp(job.from.x, endX, twigT) + randomFloat(-2.2f, 2.2f);
				float twigY = Lerp(job.from.y, endY, twigT) + randomFloat(-1.5f, 2.2f);
				clampCavePoint(twigX, twigY, (std::max)(14, minDepth - 3));

				const float sideSign = randomInt(0, 1) == 0 ? -1.0f : 1.0f;
				float twigAngle = job.angle + sideSign * randomFloat(0.64f, 1.28f) + randomFloat(-0.18f, 0.18f);
				twigAngle = std::clamp(twigAngle, 0.18f, 2.96f);
				branchJobs.push_back({ CaveNode{ twigX, twigY, 1.0f }, twigAngle, job.depth + 1, twig, false });
			}
		}

		const bool forceLobby = job.depth == 0 && job.forceLobby;
		const bool makeLobby = job.depth < 3 && (forceLobby || randomInt(0, 99) < (job.depth == 0 ? 42 : (job.depth == 1 ? 30 : 13)));
		if (makeLobby)
		{
			const float depthRatio = endY / static_cast<float>(m_blockHeight);
			const float roomRadiusX = job.depth == 0 ? randomFloat(2.05f, 3.20f + depthRatio * 0.35f) :
				(job.depth == 1 ? randomFloat(1.65f, 2.65f + depthRatio * 0.28f) :
					randomFloat(1.20f, 2.05f + depthRatio * 0.20f));
			const float roomRadiusY = job.depth == 0 ? randomFloat(1.02f, 1.72f + depthRatio * 0.24f) :
				(job.depth == 1 ? randomFloat(0.86f, 1.48f + depthRatio * 0.18f) :
					randomFloat(0.66f, 1.18f + depthRatio * 0.14f));
			CaveNode lobby = carveLobedChamber(endX, endY, roomRadiusX, roomRadiusY, false);
			caveGraphNodes.push_back(lobby);

			const int childCount = job.depth == 0 ? (job.forceLobby ? randomInt(2, 4) : randomInt(1, 3)) :
				(job.depth == 1 ? randomInt(1, 2) : randomInt(0, 1));
			for (int child = 0; child < childCount && branchJobs.size() < 160; ++child)
			{
				const float childT = childCount <= 1 ? 0.5f : static_cast<float>(child) / static_cast<float>(childCount - 1);
				const float spread = Lerp(-1.08f, 1.08f, childT);
				float childAngle = job.angle + spread + randomFloat(-0.26f, 0.26f);
				childAngle = std::clamp(childAngle, 0.16f, 2.98f);
				branchJobs.push_back({ lobby, childAngle, job.depth + 1, child, false });
			}
		}
		else
		{
			const float endRadiusX = randomFloat(1.3f, 2.4f);
			const float endRadiusY = randomFloat(0.9f, 1.6f);
			CaveNode deadEnd = carveLobedChamber(endX, endY, endRadiusX, endRadiusY, false);
			caveGraphNodes.push_back(deadEnd);
		}
	}

	const int loopConnectorCount = (std::min)(8, static_cast<int>(caveGraphNodes.size()) / 7);
	for (int connector = 0; connector < loopConnectorCount && caveGraphNodes.size() > 4; ++connector)
	{
		const int fromIndex = randomInt(0, static_cast<int>(caveGraphNodes.size()) - 3);
		const int toIndex = std::clamp(fromIndex + randomInt(2, 9), 0, static_cast<int>(caveGraphNodes.size()) - 1);
		const CaveNode& from = caveGraphNodes[fromIndex];
		const CaveNode& to = caveGraphNodes[toIndex];
		const float dx = to.x - from.x;
		const float dy = to.y - from.y;
		const float distance = std::sqrt(dx * dx + dy * dy);
		if (distance < 22.0f || distance > 80.0f || std::fabs(dy) > 38.0f)
			continue;

		carveNaturalPath(from, to.x, to.y, randomFloat(0.58f, 0.84f), false);
	}

	auto removeDetachedTerrain = [this, &surfaceHeights]()
	{
		const int tileCount = m_blockWidth * m_blockHeight;
		std::vector<unsigned char> connected(tileCount, 0);
		std::vector<int> queue;
		queue.reserve(tileCount);

		auto enqueueConnected = [this, &connected, &queue](int tileX, int tileY)
		{
			if (tileX < 0 || tileX >= m_blockWidth || tileY < 0 || tileY >= m_blockHeight)
				return;

			const int index = tileY * m_blockWidth + tileX;
			if (connected[index] != 0 || m_world.blocks[index].visible == 0)
				return;

			connected[index] = 1;
			queue.push_back(index);
		};

		for (int x = 0; x < m_blockWidth; ++x)
		{
			enqueueConnected(x, m_blockHeight - 1);
			for (int y = surfaceHeights[x]; y <= (std::min)(surfaceHeights[x] + 1, m_blockHeight - 1); ++y)
				enqueueConnected(x, y);
		}

		for (int y = 0; y < m_blockHeight; ++y)
		{
			enqueueConnected(0, y);
			enqueueConnected(m_blockWidth - 1, y);
		}

		for (size_t head = 0; head < queue.size(); ++head)
		{
			const int index = queue[head];
			const int tileX = index % m_blockWidth;
			const int tileY = index / m_blockWidth;
			enqueueConnected(tileX - 1, tileY);
			enqueueConnected(tileX + 1, tileY);
			enqueueConnected(tileX, tileY - 1);
			enqueueConnected(tileX, tileY + 1);
		}

		std::vector<unsigned char> visited = connected;
		std::vector<int> component;
		component.reserve(256);

		auto enqueueDetached = [this, &visited, &queue](int tileX, int tileY)
		{
			if (tileX < 0 || tileX >= m_blockWidth || tileY < 0 || tileY >= m_blockHeight)
				return;

			const int index = tileY * m_blockWidth + tileX;
			if (visited[index] != 0 || m_world.blocks[index].visible == 0)
				return;

			visited[index] = 1;
			queue.push_back(index);
		};

		for (int index = 0; index < tileCount; ++index)
		{
			if (visited[index] != 0 || m_world.blocks[index].visible == 0)
				continue;

			queue.clear();
			component.clear();
			visited[index] = 1;
			queue.push_back(index);

			for (size_t head = 0; head < queue.size(); ++head)
			{
				const int current = queue[head];
				component.push_back(current);
				const int tileX = current % m_blockWidth;
				const int tileY = current / m_blockWidth;
				enqueueDetached(tileX - 1, tileY);
				enqueueDetached(tileX + 1, tileY);
				enqueueDetached(tileX, tileY - 1);
				enqueueDetached(tileX, tileY + 1);
			}

			constexpr size_t MaxDetachedDebrisTiles = 28;
			if (component.size() <= MaxDetachedDebrisTiles)
			{
				for (int detachedIndex : component)
					m_world.blocks[detachedIndex].visible = 0;
			}
		}
	};

	removeDetachedTerrain();

	auto placeOreVein = [this, &surfaceHeights, &biomes, &randomInt, &randomFloat](int minDepth, int maxDepth, int steps, float radius)
	{
		int veinX = randomInt(4, m_blockWidth - 5);
		int startY = surfaceHeights[veinX] + minDepth;
		int endY = surfaceHeights[veinX] + maxDepth;
		if (startY >= m_blockHeight - 3)
			return;

		startY = std::clamp(startY, surfaceHeights[veinX] + 3, m_blockHeight - 4);
		endY = std::clamp(endY, startY, m_blockHeight - 4);
		float veinY = static_cast<float>(randomInt(startY, endY));
		float angle = randomFloat(-3.14f, 3.14f);

		for (int step = 0; step < steps; ++step)
		{
			const int minX = (std::max)(1, static_cast<int>(std::floor(veinX - radius)));
			const int maxX = (std::min)(m_blockWidth - 2, static_cast<int>(std::ceil(veinX + radius)));
			const int minY = (std::max)(surfaceHeights[veinX] + 3, static_cast<int>(std::floor(veinY - radius)));
			const int maxY = (std::min)(m_blockHeight - 2, static_cast<int>(std::ceil(veinY + radius)));

			for (int y = minY; y <= maxY; ++y)
			{
				for (int x = minX; x <= maxX; ++x)
				{
					const float dx = (static_cast<float>(x - veinX)) / radius;
					const float dy = (static_cast<float>(y) - veinY) / radius;
					if (dx * dx + dy * dy > 1.0f)
						continue;

					BlockTile& tile = m_world.blocks[y * m_blockWidth + x];
					if (tile.visible != 0 && IsStoneLikeTile(tile.tileIndex))
						tile.tileIndex = biomes[x] == BiomeIce ? BlockCrystalOre : BlockOre;
				}
			}

			angle += randomFloat(-0.55f, 0.55f);
			veinX = std::clamp(veinX + static_cast<int>(std::round(std::cos(angle) * randomFloat(0.8f, 1.8f))), 2, m_blockWidth - 3);
			veinY = std::clamp(veinY + std::sin(angle) * randomFloat(0.6f, 1.6f), static_cast<float>(surfaceHeights[veinX] + 4), static_cast<float>(m_blockHeight - 3));
		}
	};

	for (int i = 0; i < 46; ++i)
		placeOreVein(8, 42, randomInt(6, 14), randomFloat(1.1f, 2.0f));
	for (int i = 0; i < 18; ++i)
		placeOreVein(28, 78, randomInt(12, 24), randomFloat(1.8f, 3.2f));

	auto setSkyBlock = [this](int tileX, int tileY, unsigned short tileIndex)
	{
		if (!IsTileInBounds(tileX, tileY))
			return;

		BlockTile& tile = m_world.blocks[tileY * m_blockWidth + tileX];
		if (tile.visible != 0)
			return;

		tile.visible = 1;
		tile.tileIndex = tileIndex;
	};

	auto isLeafTile = [this](int tileX, int tileY)
	{
		if (!IsTileInBounds(tileX, tileY))
			return false;

		const BlockTile& tile = m_world.blocks[tileY * m_blockWidth + tileX];
		return tile.visible != 0 && tile.tileIndex == BlockLeaves;
	};

	auto hasTreeSupport = [this](int tileX, int surfaceY)
	{
		if (!IsTileInBounds(tileX, surfaceY) || !IsSolidTile(tileX, surfaceY))
			return false;

		if (!IsTileInBounds(tileX, surfaceY + 1) || !IsSolidTile(tileX, surfaceY + 1))
			return false;

		return true;
	};

	auto hasClearTrunkColumn = [this](int tileX, int surfaceY, int trunkHeight)
	{
		for (int i = 1; i <= trunkHeight; ++i)
		{
			const int trunkY = surfaceY - i;
			if (!IsTileInBounds(tileX, trunkY))
				return false;

			const BlockTile& tile = m_world.blocks[trunkY * m_blockWidth + tileX];
			if (tile.visible != 0)
				return false;
		}

		return true;
	};

	auto addLeafAnchor = [this](std::vector<int>& leafAnchors, int tileX, int tileY)
	{
		if (!IsTileInBounds(tileX, tileY))
			return;

		const int anchorIndex = tileY * m_blockWidth + tileX;
		const BlockTile& tile = m_world.blocks[anchorIndex];
		if (tile.visible != 0 && tile.tileIndex == BlockWood)
			leafAnchors.push_back(anchorIndex);
	};

	auto placeConnectedCanopy = [&](const std::vector<int>& leafAnchors, BiomeType biome)
	{
		if (leafAnchors.empty())
			return;

		const int trunkAnchorX = leafAnchors.front() % m_blockWidth;

		for (size_t anchorSlot = 0; anchorSlot < leafAnchors.size(); ++anchorSlot)
		{
			const int anchorIndex = leafAnchors[anchorSlot];
			const int anchorX = anchorIndex % m_blockWidth;
			const int anchorY = anchorIndex / m_blockWidth;
			const int direction = anchorX > trunkAnchorX ? 1 : (anchorX < trunkAnchorX ? -1 : 0);
			const bool branchCanopy = anchorSlot > 0 && direction != 0;
			const int centerX = anchorX + (branchCanopy ? direction : 0);
			const int radius = branchCanopy ? (biome == BiomeIce ? 2 : randomInt(2, 3)) : (biome == BiomeIce ? randomInt(2, 3) : randomInt(3, 4));
			const int topReach = branchCanopy ? randomInt(1, 2) : radius;
			const int bottomReach = branchCanopy ? 1 : (biome == BiomeIce ? 1 : randomInt(1, 2));

			for (int leafY = anchorY - topReach; leafY <= anchorY + bottomReach; ++leafY)
			{
				for (int leafX = centerX - radius; leafX <= centerX + radius; ++leafX)
				{
					const int dx = std::abs(leafX - centerX);
					const int dy = std::abs(leafY - anchorY);
					const int allowedWidth = radius - (dy > 1 ? dy - 1 : 0);
					if (allowedWidth < 0 || dx > allowedWidth)
						continue;
					if (branchCanopy && dx == allowedWidth && randomInt(0, 99) < 22)
						continue;

					setSkyBlock(leafX, leafY, BlockLeaves);
				}
			}
		}

		for (size_t anchorSlot = 0; anchorSlot < leafAnchors.size(); ++anchorSlot)
		{
			const int anchorIndex = leafAnchors[anchorSlot];
			const int anchorX = anchorIndex % m_blockWidth;
			const int anchorY = anchorIndex / m_blockWidth;
			const int direction = anchorX > trunkAnchorX ? 1 : (anchorX < trunkAnchorX ? -1 : 0);
			const bool branchCanopy = anchorSlot > 0 && direction != 0;
			const int centerX = anchorX + (branchCanopy ? direction : 0);
			const int fringeRadius = branchCanopy ? (biome == BiomeIce ? 3 : 4) : (biome == BiomeIce ? 3 : 4);

			for (int leafY = anchorY - fringeRadius; leafY <= anchorY + 2; ++leafY)
			{
				for (int leafX = centerX - fringeRadius; leafX <= centerX + fringeRadius; ++leafX)
				{
					if (!IsTileInBounds(leafX, leafY))
						continue;

					BlockTile& tile = m_world.blocks[leafY * m_blockWidth + leafX];
					if (tile.visible != 0)
						continue;

					const int distance = std::abs(leafX - centerX) + std::abs(leafY - anchorY);
					if (distance > fringeRadius + 1 || randomInt(0, 99) < (branchCanopy ? 70 : 64))
						continue;

					if (isLeafTile(leafX - 1, leafY) || isLeafTile(leafX + 1, leafY) ||
						isLeafTile(leafX, leafY - 1) || isLeafTile(leafX, leafY + 1))
					{
						tile.visible = 1;
						tile.tileIndex = BlockLeaves;
					}
				}
			}
		}
	};

	int nextTreeX = randomInt(2, 5);
	const int spawnNoTreeRadius = spawnSafeRadius + 14;
	for (int x = 2; x < m_blockWidth - 2; ++x)
	{
		const BiomeType biome = biomes[x];
		if (x < nextTreeX)
			continue;
		if (std::abs(x - spawnX) <= spawnNoTreeRadius)
			continue;

		const int surfaceY = surfaceHeights[x];
		if (surfaceY < 7 || surfaceY > m_blockHeight - 8)
			continue;
		if (!hasTreeSupport(x, surfaceY))
			continue;

		const int treeChance = biome == BiomeGrassland ? 68 : (biome == BiomeDesert ? 12 : 26);
		if (randomInt(0, 99) > treeChance)
			continue;

		const int trunkHeight = biome == BiomeDesert ? randomInt(3, 8) : (biome == BiomeIce ? randomInt(5, 12) : randomInt(4, 11));
		if (!hasClearTrunkColumn(x, surfaceY, trunkHeight))
			continue;

		for (int i = 1; i <= trunkHeight; ++i)
			setSkyBlock(x, surfaceY - i, BlockWood);

		if (biome != BiomeDesert)
		{
			std::vector<int> leafAnchors;
			const int leafCenterY = surfaceY - trunkHeight;
			addLeafAnchor(leafAnchors, x, leafCenterY);

			const int branchCount = trunkHeight >= 7 ? randomInt(0, biome == BiomeGrassland ? 2 : 1) : 0;
			int lastBranchDirection = 0;
			for (int branch = 0; branch < branchCount; ++branch)
			{
				const int direction = lastBranchDirection == 0 ? (randomInt(0, 1) == 0 ? -1 : 1) : -lastBranchDirection;
				lastBranchDirection = direction;

				const int branchStartY = surfaceY - randomInt(3, trunkHeight - 2);
				const int branchLength = randomInt(1, biome == BiomeGrassland ? 3 : 2);
				int branchEndX = x;
				for (int step = 1; step <= branchLength; ++step)
				{
					branchEndX = x + direction * step;
					setSkyBlock(branchEndX, branchStartY, BlockWood);
				}

				addLeafAnchor(leafAnchors, branchEndX, branchStartY);
			}

			placeConnectedCanopy(leafAnchors, biome);
		}
		else if (trunkHeight >= 5)
		{
			const int armCount = randomInt(0, 2);
			int lastArmDirection = 0;
			for (int arm = 0; arm < armCount; ++arm)
			{
				const int direction = lastArmDirection == 0 ? (randomInt(0, 1) == 0 ? -1 : 1) : -lastArmDirection;
				lastArmDirection = direction;
				const int armBaseY = surfaceY - randomInt(2, trunkHeight - 1);
				const int armX = x + direction;
				setSkyBlock(armX, armBaseY, BlockWood);
				setSkyBlock(armX, armBaseY - 1, BlockWood);
			}
		}

		nextTreeX = x + (biome == BiomeGrassland ? randomInt(7, 14) : (biome == BiomeDesert ? randomInt(12, 22) : randomInt(10, 18)));
	}

	const int spawnAirClearRadius = 3;
	for (int y = 0; y < spawnSurfaceY; ++y)
	{
		for (int x = spawnX - spawnAirClearRadius; x <= spawnX + spawnAirClearRadius; ++x)
		{
			if (!IsTileInBounds(x, y))
				continue;

			m_world.blocks[y * m_blockWidth + x].visible = 0;
		}
	}
	for (int x = spawnX - spawnSafeRadius; x <= spawnX + spawnSafeRadius; ++x)
	{
		if (x < 0 || x >= m_blockWidth)
			continue;

		for (int y = spawnSurfaceY; y <= spawnSurfaceY + 6 && y < m_blockHeight; ++y)
		{
			BlockTile& tile = m_world.blocks[y * m_blockWidth + x];
			tile.visible = 1;
			const int depth = y - spawnSurfaceY;
			const float layerNoise = FractalNoise2D(x * 0.11f, y * 0.11f, seed + 503u, 3, 2.0f, 0.55f);
			tile.tileIndex = GetTerrainTileForBiome(BiomeGrassland, depth, layerNoise);
		}
	}

	const int tableX = (std::min)(m_blockWidth - 3, spawnX + 6);
	const int tableY = surfaceHeights[tableX] - 1;
	if (IsTileInBounds(tableX, tableY))
	{
		BlockTile& tableTile = m_world.blocks[tableY * m_blockWidth + tableX];
		tableTile.visible = 1;
		tableTile.tileIndex = BlockCraftingTable;
	}

	auto canSpawnMonsterAt = [this](float centerX, float centerY)
	{
		if (IsAABBBlocked(GetMonsterAABB(centerX, centerY), 0.25f))
			return false;

		const float minDistance = m_tileSize * 2.4f;
		for (const MonsterState& existing : m_monsters)
		{
			if (!existing.alive)
				continue;

			const float dx = existing.x - centerX;
			const float dy = existing.y - centerY;
			if (dx * dx + dy * dy < minDistance * minDistance)
				return false;
		}

		return true;
	};

	auto spawnMonsterAtFloor = [this, &canSpawnMonsterAt, spawnX, &randomFloat](int tileX, int floorTileY, bool underground)
	{
		if (!IsTileInBounds(tileX, floorTileY) || !IsSolidTile(tileX, floorTileY))
			return false;

		const float centerX = m_worldOriginX + tileX * m_tileSize;
		const float centerY = TileTop(floorTileY) + MonsterCollisionHeight * 0.5f + 0.02f;
		if (!canSpawnMonsterAt(centerX, centerY))
			return false;

		MonsterState monster;
		monster.x = centerX;
		monster.y = centerY;
		monster.homeX = centerX;
		monster.lastX = centerX;
		monster.facing = tileX < spawnX ? 1 : -1;
		monster.health = underground ? 48 : 40;
		monster.maxHealth = monster.health;
		monster.biome = m_world.biomes.empty() ? static_cast<unsigned char>(BiomeGrassland) :
			static_cast<unsigned char>(std::clamp(static_cast<int>(m_world.biomes[tileX]), 0, BackgroundBiomeCount - 1));
		monster.alive = true;
		monster.onGround = true;
		monster.underground = underground;
		monster.aiTimer = randomFloat(0.45f, 1.40f);
		monster.jumpCooldown = randomFloat(0.10f, 0.85f);
		monster.animationTime = randomFloat(0.0f, 2.0f);
		m_monsters.push_back(monster);
		return true;
	};

	const int surfaceMonsterCount = 10;
	for (int i = 0; i < surfaceMonsterCount; ++i)
	{
		for (int attempt = 0; attempt < 80; ++attempt)
		{
			int monsterTileX = randomInt(5, m_blockWidth - 6);
			if (std::abs(monsterTileX - spawnX) < 15)
				continue;

			const int monsterSurfaceY = surfaceHeights[monsterTileX];
			if (spawnMonsterAtFloor(monsterTileX, monsterSurfaceY, false))
				break;
		}
	}

	const int undergroundMonsterCount = 18;
	for (int i = 0; i < undergroundMonsterCount; ++i)
	{
		for (int attempt = 0; attempt < 220; ++attempt)
		{
			const int tileX = randomInt(5, m_blockWidth - 6);
			if (std::abs(tileX - spawnX) < 12)
				continue;

			const int minFloorY = surfaceHeights[tileX] + 16;
			const int maxFloorY = m_blockHeight - 7;
			if (minFloorY >= maxFloorY)
				continue;

			const int floorY = randomInt(minFloorY, maxFloorY);
			if (!IsSolidTile(tileX, floorY) ||
				IsSolidTile(tileX, floorY - 1) ||
				IsSolidTile(tileX, floorY - 2) ||
				IsSolidTile(tileX - 1, floorY - 1) ||
				IsSolidTile(tileX + 1, floorY - 1))
				continue;

			if (spawnMonsterAtFloor(tileX, floorY, true))
				break;
		}
	}
	RebuildMonsterSpatialGrid();
	m_monsterQueryScratch.reserve(m_monsters.size());
	m_monsterOverlapScratch.reserve(m_monsters.size());
	InitializeBlockChunkCache();

	m_inventory.Clear();
	const int teleportPotionSlot = AddInventoryItem(SlotTeleportPotion, 1);
	AddInventoryItem(SlotHealthPotion, 2);
	AddInventoryItem(SlotSpeedPotion, 1);
	AddInventoryItem(SlotJumpPotion, 1);
	AddInventoryItem(SlotGuardPotion, 1);
	AddInventoryItem(SlotIronHelmet, 1);
	AddInventoryItem(SlotIronArmor, 1);
	AddInventoryItem(SlotSwiftBoots, 1);
	AddInventoryItem(SlotLuckyCharm, 1);
	AddInventoryItem(SlotPistol, 1);
	AddInventoryItem(SlotBullet, 24);
	AddInventoryItem(SlotBow, 1);
	AddInventoryItem(SlotArrow, 24);
	m_inventory.SetSelectedSlot(teleportPotionSlot >= 0 ? teleportPotionSlot : 0);
	m_playerHealth = GetPlayerMaxHealth();
	m_playerInvulnerableTimer = 0.0f;
	m_playerHurtFlashTimer = 0.0f;
	m_playerKnockbackTimer = 0.0f;
	m_playerKnockbackCooldownTimer = 0.0f;
	m_speedPotionTimer = 0.0f;
	m_jumpPotionTimer = 0.0f;
	m_guardPotionTimer = 0.0f;
	m_attackCooldown = 0.0f;
	m_attackTimer = 0.0f;
	SetStatusText("텔레포트 물약", 2.0f);

	const float groundTop = TileTop(spawnSurfaceY);
	m_player.x = m_worldOriginX + spawnX * m_tileSize;
	m_player.y = groundTop + (m_playerCollisionHeight * 0.5f) + 0.02f;
	m_playerSpawnX = m_player.x;
	m_playerSpawnY = m_player.y;
	m_player.velocityX = 0.0f;
	m_player.velocityY = 0.0f;
	m_player.animationTime = 0.0f;
	m_player.facing = 1;
	m_player.onGround = true;
	m_cameraX = m_player.x;
	m_cameraY = m_player.y;
	UpdateMapReveal();
	if (!m_networkState.pendingTileEdits.empty())
	{
		const std::vector<NetworkTileEditState> pendingEdits = m_networkState.pendingTileEdits;
		m_networkState.pendingTileEdits.clear();
		for (const NetworkTileEditState& edit : pendingEdits)
			ApplyNetworkTileEdit(edit.tileX, edit.tileY, edit.tileIndex, edit.visible);
	}
	if (!m_networkState.pendingDroppedItems.empty())
	{
		const std::vector<DroppedItemState> pendingItems = m_networkState.pendingDroppedItems;
		m_networkState.pendingDroppedItems.clear();
		for (const DroppedItemState& item : pendingItems)
		{
			ApplyNetworkDroppedItemState(item.networkId, 0, item.pickupPlayerId, item.x, item.y, item.velocityX, item.velocityY,
				item.pickupDelay, item.tileIndex, item.amount, item.alive ? 1 : 0);
		}
	}
}


