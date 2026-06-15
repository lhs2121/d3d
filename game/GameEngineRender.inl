// Included by GameEngine.cpp. Shares its private class declarations and file-local helpers.

void GameEngine::DrawBackground()
{
	const float surfaceWorldY = GetSurfaceWorldYAt(m_cameraX);
	const float undergroundDepth = surfaceWorldY - m_cameraY;
	const float undergroundAlpha = SmoothStep(Clamp01((undergroundDepth - 6.0f) / 96.0f));
	constexpr int BlendTiles = 72;
	float biomeWeights[BackgroundBiomeCount] = {};
	float totalWeight = 0.0f;

	const float leftEdge = m_worldOriginX - m_tileSize * 0.5f;
	const float centerTileX = (m_cameraX - leftEdge) / m_tileSize;
	const int firstTileX = static_cast<int>(std::floor(centerTileX)) - BlendTiles - 1;
	const int lastTileX = static_cast<int>(std::ceil(centerTileX)) + BlendTiles + 1;

	for (int sampleTileX = firstTileX; sampleTileX <= lastTileX; ++sampleTileX)
	{
		const float distance = std::fabs(static_cast<float>(sampleTileX) - centerTileX);
		if (distance > static_cast<float>(BlendTiles + 1))
			continue;

		const int tileX = std::clamp(sampleTileX, 0, m_blockWidth - 1);
		const int biome = m_world.biomes.empty() ? BiomeGrassland : std::clamp(static_cast<int>(m_world.biomes[tileX]), 0, BackgroundBiomeCount - 1);
		const float blend = 1.0f - distance / static_cast<float>(BlendTiles + 1);
		const float weight = SmoothStep(Clamp01(blend));
		biomeWeights[biome] += weight;
		totalWeight += weight;
	}

	if (totalWeight <= 0.0f)
	{
		DrawSkyBackground(BiomeGrassland, 1.0f);
		return;
	}

	for (int biome = 0; biome < BackgroundBiomeCount; ++biome)
		biomeWeights[biome] /= totalWeight;

	auto drawWeightedSurfaceLayer = [&](int layerIndex)
	{
		float accumulatedWeight = 0.0f;
		for (int biome = 0; biome < BackgroundBiomeCount; ++biome)
		{
			const float weight = biomeWeights[biome];
			if (weight <= 0.001f)
				continue;

			accumulatedWeight += weight;
			const float alpha = weight / accumulatedWeight;
			const BackgroundImageLayer& layer = SurfaceBackgroundLayers[biome][layerIndex];
			DrawBackgroundImageLayer(layer.textureFile, alpha * layer.alpha, layer.parallaxX, layer.parallaxY, layer.depth);
		}
	};

	for (int layerIndex = 0; layerIndex < SurfaceBackgroundLayerCount; ++layerIndex)
		drawWeightedSurfaceLayer(layerIndex);

	const float caveCoverage = Clamp01(undergroundAlpha * (0.86f + Clamp01((undergroundDepth - 24.0f) / 360.0f) * 0.14f));
	if (caveCoverage <= 0.01f)
		return;

	auto drawWeightedCaveLayer = [&](int layerIndex)
	{
		float accumulatedCoverage = 1.0f - caveCoverage;
		for (int biome = 0; biome < BackgroundBiomeCount; ++biome)
		{
			const float coverage = caveCoverage * biomeWeights[biome];
			if (coverage <= 0.001f)
				continue;

			accumulatedCoverage += coverage;
			const float alpha = coverage / accumulatedCoverage;
			const BackgroundImageLayer& layer = CaveBackgroundLayers[biome][layerIndex];
			DrawBackgroundImageLayer(layer.textureFile, alpha * layer.alpha, layer.parallaxX, layer.parallaxY, layer.depth);
		}
	};

	for (int layerIndex = 0; layerIndex < CaveBackgroundLayerCount; ++layerIndex)
		drawWeightedCaveLayer(layerIndex);
}

void GameEngine::DrawBackgroundImageLayer(const WCHAR* textureFile, float alpha, float parallaxX, float parallaxY, float depth)
{
	if (textureFile == nullptr || alpha <= 0.0f)
		return;

	const float viewHalfWidth = GetViewHalfWidth();
	const float viewHalfHeight = GetViewHalfHeight();
	const float layerWidth = viewHalfWidth * 2.0f + 320.0f;
	const float layerHeight = viewHalfHeight * 2.32f;
	float wrappedOffset = std::fmod(m_cameraX * parallaxX, layerWidth);
	if (wrappedOffset < 0.0f)
		wrappedOffset += layerWidth;

	float centerX = -wrappedOffset;
	while (centerX > -viewHalfWidth - layerWidth * 0.5f)
		centerX -= layerWidth;

	for (; centerX < viewHalfWidth + layerWidth * 0.5f; centerX += layerWidth)
	{
		SpriteDesc desc;
		desc.textureFile = textureFile;
		desc.positionX = centerX;
		desc.positionY = -m_cameraY * parallaxY;
		desc.width = layerWidth;
		desc.height = layerHeight;
		desc.colorA = Clamp01(alpha);
		desc.depth = depth;
		m_renderer->DrawSprite(desc);
	}
}

void GameEngine::DrawSkyBackground(int biome, float alpha)
{
	alpha = Clamp01(alpha);
	if (alpha <= 0.0f)
		return;

	const int textureIndex = std::clamp(biome, 0, BackgroundBiomeCount - 1);
	for (const BackgroundImageLayer& layer : SurfaceBackgroundLayers[textureIndex])
		DrawBackgroundImageLayer(layer.textureFile, alpha * layer.alpha, layer.parallaxX, layer.parallaxY, layer.depth);
	return;

	const float viewHalfWidth = GetViewHalfWidth();
	const float viewHalfHeight = GetViewHalfHeight();
	const float fullWidth = viewHalfWidth * 2.35f;
	const float bandHeight = (viewHalfHeight * 2.25f) / 10.0f;
	const float topY = viewHalfHeight + bandHeight * 0.18f;
	float topR = 0.23f;
	float topG = 0.44f;
	float topB = 0.68f;
	float bottomR = 0.48f;
	float bottomG = 0.62f;
	float bottomB = 0.84f;
	float hazeR = 0.19f;
	float hazeG = 0.27f;
	float hazeB = 0.30f;

	if (biome == BiomeDesert)
	{
		topR = 0.46f; topG = 0.58f; topB = 0.72f;
		bottomR = 0.91f; bottomG = 0.68f; bottomB = 0.36f;
		hazeR = 0.61f; hazeG = 0.45f; hazeB = 0.24f;
	}
	else if (biome == BiomeIce)
	{
		topR = 0.16f; topG = 0.31f; topB = 0.55f;
		bottomR = 0.62f; bottomG = 0.78f; bottomB = 0.89f;
		hazeR = 0.42f; hazeG = 0.56f; hazeB = 0.66f;
	}

	for (int band = 0; band < 10; ++band)
	{
		const float ratio = static_cast<float>(band) / 9.0f;
		const float colorR = Lerp(topR, bottomR, ratio);
		const float colorG = Lerp(topG, bottomG, ratio);
		const float colorB = Lerp(topB, bottomB, ratio);
		DrawSolidRect(0.0f, topY - band * bandHeight, fullWidth, bandHeight + 1.0f,
			colorR, colorG, colorB, alpha, 6.0f);
	}

	DrawSolidRect(0.0f, -viewHalfHeight + 32.0f, fullWidth, 96.0f,
		hazeR, hazeG, hazeB, 0.78f * alpha, 5.8f);

	if (biome == BiomeDesert)
	{
		auto drawDuneLayer = [this, viewHalfWidth, viewHalfHeight, alpha](float baseY, float spacing, float parallaxX, float colorR, float colorG, float colorB, float colorA, float depth)
		{
			const float wrappedOffset = std::fmod(m_cameraX * parallaxX, spacing);
			const int firstDune = static_cast<int>(std::floor((-viewHalfWidth - spacing - wrappedOffset) / spacing));
			const int lastDune = static_cast<int>(std::ceil((viewHalfWidth + spacing - wrappedOffset) / spacing));
			for (int dune = firstDune; dune <= lastDune; ++dune)
			{
				const float centerX = dune * spacing - wrappedOffset;
				const float duneHeight = viewHalfHeight * (0.22f + static_cast<float>((dune * 17) % 19) / 100.0f);
				const int steps = 12;
				for (int step = 0; step < steps; ++step)
				{
					const float ratio = static_cast<float>(step) / static_cast<float>(steps);
					const float width = spacing * (1.05f - ratio * 0.70f);
					const float y = baseY + step * (duneHeight / steps);
					DrawSolidRect(centerX + ratio * spacing * 0.16f, y, width, duneHeight / steps + 1.0f,
						colorR + ratio * 0.05f, colorG + ratio * 0.04f, colorB + ratio * 0.02f, colorA * alpha, depth);
				}
			}
		};

		DrawCloudLayer(viewHalfHeight * 0.56f, 380.0f, 0.035f, 0.012f, 0.96f, 0.84f, 0.62f, 0.20f * alpha, 5.2f);
		drawDuneLayer(-viewHalfHeight * 0.35f, 300.0f, 0.10f, 0.70f, 0.52f, 0.26f, 0.86f, 4.0f);
		drawDuneLayer(-viewHalfHeight * 0.55f, 230.0f, 0.22f, 0.54f, 0.38f, 0.20f, 0.94f, 3.0f);

		const float shimmerSpacing = 86.0f;
		const float shimmerOffset = std::fmod(m_cameraX * 0.18f, shimmerSpacing);
		for (int line = -7; line <= 7; ++line)
		{
			const float shimmerX = line * shimmerSpacing - shimmerOffset;
			const float shimmerY = -viewHalfHeight * 0.02f + static_cast<float>((line * 29) % 47);
			DrawSolidRect(shimmerX, shimmerY, 42.0f, 2.0f, 1.0f, 0.88f, 0.58f, 0.13f * alpha, 2.1f);
		}
		return;
	}

	if (biome == BiomeIce)
	{
		const float auroraOffset = std::fmod(m_cameraX * 0.035f, 360.0f);
		for (int ribbon = -2; ribbon <= 3; ++ribbon)
		{
			const float ribbonX = ribbon * 360.0f - auroraOffset;
			const float ribbonY = viewHalfHeight * (0.48f + 0.05f * static_cast<float>((ribbon * 11) % 4));
			DrawSolidRect(ribbonX, ribbonY, 280.0f, 12.0f,
				0.46f, 0.92f, 0.78f, 0.15f * alpha, 5.35f);
			DrawSolidRect(ribbonX + 60.0f, ribbonY - 18.0f, 210.0f, 7.0f,
				0.36f, 0.76f, 1.0f, 0.11f * alpha, 5.30f);
		}

		DrawCloudLayer(viewHalfHeight * 0.54f, 330.0f, 0.045f, 0.018f, 0.82f, 0.92f, 0.96f, 0.24f * alpha, 5.1f);
		DrawMountainLayer(-viewHalfHeight * 0.23f, 210.0f, 260.0f, 0.10f, 0.035f,
			0.45f, 0.58f, 0.70f, 0.82f * alpha, 4.0f);
		DrawMountainLayer(-viewHalfHeight * 0.48f, 280.0f, 330.0f, 0.20f, 0.060f,
			0.30f, 0.43f, 0.56f, 0.92f * alpha, 3.0f);

		const float snowOffset = std::fmod(m_cameraX * 0.12f + m_cameraY * 0.04f, 72.0f);
		for (int flake = -10; flake <= 11; ++flake)
		{
			const float snowX = flake * 72.0f - snowOffset + static_cast<float>((flake * 31) % 23);
			const float snowY = viewHalfHeight * 0.50f - std::fmod(static_cast<float>(flake * 53), viewHalfHeight * 1.45f);
			DrawSolidRect(snowX, snowY, 3.0f, 3.0f, 0.92f, 0.98f, 1.0f, 0.28f * alpha, 2.1f);
		}
		return;
	}

	DrawCloudLayer(viewHalfHeight * 0.58f, 260.0f, 0.04f, 0.015f, 0.86f, 0.91f, 0.93f, 0.34f * alpha, 5.2f);
	DrawCloudLayer(viewHalfHeight * 0.40f, 340.0f, 0.07f, 0.025f, 0.90f, 0.94f, 0.95f, 0.24f * alpha, 5.0f);

	DrawMountainLayer(-viewHalfHeight * 0.20f, 185.0f, 260.0f, 0.10f, 0.035f,
		0.29f, 0.39f, 0.51f, 0.78f * alpha, 4.0f);
	DrawMountainLayer(-viewHalfHeight * 0.39f, 245.0f, 310.0f, 0.18f, 0.060f,
		0.18f, 0.27f, 0.36f, 0.88f * alpha, 3.0f);

	const float treeSpacing = 42.0f;
	const float treeOffset = std::fmod(m_cameraX * 0.26f, treeSpacing);
	const int firstTree = static_cast<int>(std::floor((-viewHalfWidth - treeSpacing - treeOffset) / treeSpacing));
	const int lastTree = static_cast<int>(std::ceil((viewHalfWidth + treeSpacing - treeOffset) / treeSpacing));
	for (int tree = firstTree; tree <= lastTree; ++tree)
	{
		const float treeX = tree * treeSpacing - treeOffset;
		const float treeBaseY = -viewHalfHeight * 0.60f + static_cast<float>((tree * 17) % 19);
		const float treeHeight = 34.0f + static_cast<float>((tree * 13) % 17);
		DrawSolidRect(treeX, treeBaseY + treeHeight * 0.32f, 5.0f, treeHeight,
			0.12f, 0.10f, 0.07f, 0.55f * alpha, 2.2f);
		DrawSolidRect(treeX, treeBaseY + treeHeight * 0.78f, 28.0f, 20.0f,
			0.12f, 0.34f, 0.20f, 0.58f * alpha, 2.1f);
		DrawSolidRect(treeX - 8.0f, treeBaseY + treeHeight * 0.62f, 20.0f, 16.0f,
			0.09f, 0.26f, 0.16f, 0.45f * alpha, 2.0f);
	}
}

void GameEngine::DrawUndergroundBackground(float undergroundDepth, int biome, float alpha)
{
	alpha = Clamp01(alpha);
	if (alpha <= 0.0f)
		return;

	const int textureIndex = std::clamp(biome, 0, BackgroundBiomeCount - 1);
	const float depthRatio = Clamp01((undergroundDepth - 24.0f) / 360.0f);
	const float depthAlpha = alpha * (0.86f + depthRatio * 0.14f);
	for (const BackgroundImageLayer& layer : CaveBackgroundLayers[textureIndex])
		DrawBackgroundImageLayer(layer.textureFile, depthAlpha * layer.alpha, layer.parallaxX, layer.parallaxY, layer.depth);
	return;

	const float viewHalfWidth = GetViewHalfWidth();
	const float viewHalfHeight = GetViewHalfHeight();
	const float fullWidth = viewHalfWidth * 2.35f;
	const float fullHeight = viewHalfHeight * 2.35f;
	const float bandHeight = fullHeight / 8.0f;
	const float topY = viewHalfHeight + bandHeight * 0.2f;
	float baseR = 0.085f;
	float baseG = 0.078f;
	float baseB = 0.082f;
	float wallR = 0.105f;
	float wallG = 0.100f;
	float wallB = 0.112f;
	float glowR = 0.52f;
	float glowG = 0.78f;
	float glowB = 0.46f;

	if (biome == BiomeDesert)
	{
		baseR = 0.135f; baseG = 0.096f; baseB = 0.064f;
		wallR = 0.215f; wallG = 0.145f; wallB = 0.075f;
		glowR = 0.95f; glowG = 0.58f; glowB = 0.22f;
	}
	else if (biome == BiomeIce)
	{
		baseR = 0.050f; baseG = 0.075f; baseB = 0.105f;
		wallR = 0.075f; wallG = 0.120f; wallB = 0.155f;
		glowR = 0.38f; glowG = 0.86f; glowB = 1.0f;
	}

	for (int band = 0; band < 8; ++band)
	{
		const float ratio = static_cast<float>(band) / 7.0f;
		DrawSolidRect(0.0f, topY - band * bandHeight, fullWidth, bandHeight + 1.0f,
			baseR - depthRatio * 0.025f + ratio * 0.018f,
			baseG - depthRatio * 0.020f + ratio * 0.014f,
			baseB - depthRatio * 0.015f + ratio * 0.020f,
			alpha,
			6.0f);
	}

	DrawSolidRect(0.0f, viewHalfHeight * 0.54f, fullWidth, 70.0f,
		baseR * 0.62f, baseG * 0.62f, baseB * 0.70f, (0.42f + depthRatio * 0.20f) * alpha, 5.7f);
	DrawSolidRect(0.0f, -viewHalfHeight * 0.78f, fullWidth, 120.0f,
		baseR * 0.38f, baseG * 0.42f, baseB * 0.48f, (0.62f + depthRatio * 0.20f) * alpha, 5.7f);

	DrawCaveWallLayer(-viewHalfHeight * 0.28f, 210.0f, 0.08f, 0.035f,
		wallR, wallG, wallB, 0.82f * alpha, 4.2f);
	DrawCaveWallLayer(-viewHalfHeight * 0.48f, 260.0f, 0.16f, 0.060f,
		wallR * 0.72f, wallG * 0.76f, wallB * 0.82f, 0.90f * alpha, 3.3f);
	DrawCaveWallLayer(-viewHalfHeight * 0.66f, 170.0f, 0.27f, 0.090f,
		wallR * 0.44f, wallG * 0.52f, wallB * 0.58f, 0.94f * alpha, 2.3f);

	const float emberSpacing = biome == BiomeIce ? 128.0f : 170.0f;
	const float emberOffset = std::fmod(m_cameraX * 0.21f, emberSpacing);
	for (int ember = -5; ember <= 5; ++ember)
	{
		const float emberX = ember * emberSpacing - emberOffset + static_cast<float>((ember * 31) % 47);
		const float emberY = -viewHalfHeight * 0.12f + std::fmod(m_cameraY * 0.06f + ember * 53.0f, viewHalfHeight * 1.35f) - viewHalfHeight * 0.62f;
		const float size = biome == BiomeIce ? 5.0f : (biome == BiomeDesert ? 4.0f : 3.5f);
		DrawSolidRect(emberX, emberY, size, size,
			glowR, glowG, glowB, (0.16f + depthRatio * 0.10f) * alpha, 1.8f);
		if (biome == BiomeIce)
		{
			DrawSolidRect(emberX, emberY + 7.0f, 2.0f, 12.0f,
				glowR, glowG, glowB, 0.10f * alpha, 1.7f);
		}
	}
}

void GameEngine::DrawMountainLayer(float baseY, float height, float spacing, float parallaxX, float parallaxY, float colorR, float colorG, float colorB, float colorA, float depth)
{
	const float viewHalfWidth = GetViewHalfWidth();
	const float wrappedOffset = std::fmod(m_cameraX * parallaxX, spacing);
	const float verticalOffset = -m_cameraY * parallaxY;
	const int firstMountain = static_cast<int>(std::floor((-viewHalfWidth - spacing - wrappedOffset) / spacing));
	const int lastMountain = static_cast<int>(std::ceil((viewHalfWidth + spacing - wrappedOffset) / spacing));
	const int stepCount = 14;
	const float stepHeight = height / stepCount;
	const float bottomWidth = spacing * 0.95f;

	for (int mountain = firstMountain; mountain <= lastMountain; ++mountain)
	{
		const float centerX = mountain * spacing - wrappedOffset;
		const float peakOffset = static_cast<float>((mountain * 37) % 53) - 26.0f;
		const float mountainHeight = height + peakOffset;
		const float localStepHeight = mountainHeight / stepCount;
		const float layerBaseY = baseY + verticalOffset + static_cast<float>((mountain * 19) % 35);

		for (int step = 0; step < stepCount; ++step)
		{
			const float stepRatio = static_cast<float>(step) / static_cast<float>(stepCount);
			const float width = bottomWidth * (1.0f - stepRatio * 0.86f);
			const float y = layerBaseY + step * localStepHeight + localStepHeight * 0.5f;
			const float shade = stepRatio * 0.08f;
			DrawSolidRect(centerX, y, width, localStepHeight + 1.0f,
				colorR + shade, colorG + shade, colorB + shade, colorA, depth);
		}

		const float snowWidth = bottomWidth * 0.20f;
		const float snowY = layerBaseY + mountainHeight - localStepHeight * 1.6f;
		DrawSolidRect(centerX, snowY, snowWidth, localStepHeight * 1.8f,
			0.78f, 0.84f, 0.86f, colorA * 0.52f, depth - 0.1f);
	}
}

void GameEngine::DrawCloudLayer(float y, float spacing, float parallaxX, float parallaxY, float colorR, float colorG, float colorB, float colorA, float depth)
{
	const float viewHalfWidth = GetViewHalfWidth();
	const float wrappedOffset = std::fmod(m_cameraX * parallaxX, spacing);
	const float verticalOffset = -m_cameraY * parallaxY;
	const int firstCloud = static_cast<int>(std::floor((-viewHalfWidth - spacing - wrappedOffset) / spacing));
	const int lastCloud = static_cast<int>(std::ceil((viewHalfWidth + spacing - wrappedOffset) / spacing));

	for (int cloud = firstCloud; cloud <= lastCloud; ++cloud)
	{
		const float centerX = cloud * spacing - wrappedOffset;
		const float cloudY = y + verticalOffset + static_cast<float>((cloud * 23) % 45);
		const float widthScale = 0.82f + static_cast<float>((cloud * 13) % 27) / 100.0f;

		DrawSolidRect(centerX - 38.0f * widthScale, cloudY, 78.0f * widthScale, 18.0f,
			colorR * 0.96f, colorG * 0.98f, colorB, colorA, depth);
		DrawSolidRect(centerX + 8.0f * widthScale, cloudY + 10.0f, 92.0f * widthScale, 24.0f,
			colorR, colorG, colorB, colorA, depth - 0.1f);
		DrawSolidRect(centerX + 52.0f * widthScale, cloudY - 4.0f, 70.0f * widthScale, 16.0f,
			colorR * 0.92f, colorG * 0.95f, colorB * 0.98f, colorA * 0.85f, depth);
	}
}

void GameEngine::DrawCaveWallLayer(float baseY, float spacing, float parallaxX, float parallaxY, float colorR, float colorG, float colorB, float colorA, float depth)
{
	const float viewHalfWidth = GetViewHalfWidth();
	const float viewHalfHeight = GetViewHalfHeight();
	const float wrappedOffset = std::fmod(m_cameraX * parallaxX, spacing);
	const float verticalOffset = -m_cameraY * parallaxY;
	const int firstColumn = static_cast<int>(std::floor((-viewHalfWidth - spacing - wrappedOffset) / spacing));
	const int lastColumn = static_cast<int>(std::ceil((viewHalfWidth + spacing - wrappedOffset) / spacing));

	for (int column = firstColumn; column <= lastColumn; ++column)
	{
		const float centerX = column * spacing - wrappedOffset + static_cast<float>((column * 29) % 41);
		const float width = spacing * (0.34f + static_cast<float>((column * 17) % 19) / 100.0f);
		const float height = viewHalfHeight * (0.46f + static_cast<float>((column * 23) % 31) / 100.0f);
		const float centerY = baseY + verticalOffset + static_cast<float>((column * 31) % 73) - height * 0.12f;

		DrawSolidRect(centerX, centerY, width, height,
			colorR, colorG, colorB, colorA, depth);

		DrawSolidRect(centerX - width * 0.24f, centerY + height * 0.40f, width * 0.34f, height * 0.28f,
			colorR + 0.020f, colorG + 0.020f, colorB + 0.025f, colorA * 0.62f, depth - 0.08f);
		DrawSolidRect(centerX + width * 0.18f, centerY - height * 0.35f, width * 0.28f, height * 0.35f,
			colorR - 0.018f, colorG - 0.016f, colorB - 0.012f, colorA * 0.70f, depth - 0.06f);

		const float capY = viewHalfHeight - std::fmod(m_cameraY * parallaxY + column * 47.0f, 130.0f);
		DrawSolidRect(centerX, capY, width * 0.52f, 46.0f,
			colorR + 0.012f, colorG + 0.010f, colorB + 0.014f, colorA * 0.48f, depth - 0.12f);
	}
}

void GameEngine::DrawWorld()
{
	const UiRect gameViewport = GetGameViewportRect();
	if (m_renderer != nullptr)
		m_renderer->SetViewportRect(gameViewport.left, gameViewport.top, gameViewport.width, gameViewport.height);

	DrawBackground();

	BlockGridDesc gridDesc;
	gridDesc.textureFile = BlockAtlasTexture;
	gridDesc.tiles = m_world.blocks.data();
	gridDesc.width = m_blockWidth;
	gridDesc.height = m_blockHeight;
	gridDesc.atlasColumns = BlockAtlasColumns;
	gridDesc.atlasRows = BlockAtlasRows;
	gridDesc.tileSize = m_tileSize;
	gridDesc.originX = m_worldOriginX - m_cameraX;
	gridDesc.originY = m_worldOriginY - m_cameraY;
	gridDesc.chunkVersions = m_blockChunkVersions.empty() ? nullptr : m_blockChunkVersions.data();
	gridDesc.chunkSizeTiles = m_blockChunkSizeTiles;
	gridDesc.chunkColumns = m_blockChunkColumns;
	gridDesc.chunkRows = m_blockChunkRows;
	gridDesc.gridVersion = m_blockGridVersion;
	gridDesc.atlasTileRemap = BlockAtlasTileRemap;
	gridDesc.atlasTileRemapCount = static_cast<int>(sizeof(BlockAtlasTileRemap) / sizeof(BlockAtlasTileRemap[0]));

	m_renderer->DrawBlockGrid(gridDesc);
	DrawBlockCracks();
	DrawHoveredBlockOutline();
	DrawLeafParticles();
	DrawDroppedItems();
	DrawProjectiles();
	DrawMonsters();
	DrawRemotePlayers();

	const bool wantsLeft = IsKeyHeld(KeyA);
	const bool wantsRight = IsKeyHeld(KeyD);
	const bool isMoving = wantsLeft != wantsRight;
	const float playerSpriteYOffset =
		-m_playerCollisionHeight * 0.5f -
		m_playerDrawHeight * 0.5f +
		(PlayerSpriteFootPixelY / PlayerSpriteFramePixelHeight) * m_playerDrawHeight;

	SpriteDesc playerDesc;
	playerDesc.positionX = m_player.x - m_cameraX;
	playerDesc.positionY = m_player.y - m_cameraY + playerSpriteYOffset;
	playerDesc.width = m_playerDrawWidth;
	playerDesc.height = m_playerDrawHeight;
	playerDesc.flipX = m_player.facing < 0 ? 1 : 0;
	playerDesc.depth = -1.0f;
	playerDesc.atlasRows = PlayerSpriteRows;

	const WCHAR* playerTexture = PlayerIdleTexture;
	int playerColumns = PlayerIdleFrames;
	int playerFrame = static_cast<int>(m_player.animationTime / 0.24f) % PlayerIdleFrames;
	if (m_attackTimer > 0.0f)
	{
		const float attackProgress = 1.0f - Clamp01(m_attackTimer / PlayerAttackDuration);
		const float lunge = std::sin(attackProgress * 3.1415926f);
		playerDesc.positionX += m_player.facing * (3.0f + lunge * 4.5f);
		playerDesc.positionY -= lunge * 1.4f;
		playerDesc.rotationRadians = m_player.facing * (0.035f + lunge * 0.060f);
		playerTexture = PlayerAttackTexture;
		playerColumns = PlayerAttackFrames;
		playerFrame = std::clamp(static_cast<int>(attackProgress * static_cast<float>(PlayerAttackFrames)), 0, PlayerAttackFrames - 1);
	}
	else if (m_playerHurtFlashTimer > 0.0f)
	{
		playerTexture = PlayerHitTexture;
		playerColumns = PlayerHitFrames;
		playerFrame = static_cast<int>(m_player.animationTime / 0.10f) % PlayerHitFrames;
	}
	else if (!m_player.onGround)
	{
		if (m_player.velocityY < -8.0f)
		{
			playerTexture = PlayerFallTexture;
			playerColumns = PlayerFallFrames;
			playerFrame = static_cast<int>(m_player.animationTime / 0.13f) % PlayerFallFrames;
		}
		else
		{
			playerTexture = PlayerJumpTexture;
			playerColumns = PlayerJumpFrames;
			playerFrame = static_cast<int>(m_player.animationTime / 0.13f) % PlayerJumpFrames;
		}
	}
	else if (isMoving)
	{
		playerTexture = PlayerRunTexture;
		playerColumns = PlayerRunFrames;
		playerFrame = static_cast<int>(m_player.animationTime / 0.11f) % PlayerRunFrames;
	}
	playerDesc.textureFile = playerTexture;
	playerDesc.atlasColumns = playerColumns;
	playerDesc.tileIndex = playerFrame;

	if (m_playerHurtFlashTimer > 0.0f)
	{
		const float flashFade = Clamp01(m_playerHurtFlashTimer / PlayerHurtFlashDuration);
		const bool flashOn = (static_cast<int>(m_playerHurtFlashTimer * 30.0f) % 2) == 0;
		const float redPulse = (flashOn ? 0.78f : 0.28f) * flashFade;
		playerDesc.colorR = 1.0f + redPulse * 0.85f;
		playerDesc.colorG = 1.0f - redPulse * 0.58f;
		playerDesc.colorB = 1.0f - redPulse * 0.58f;
	}

	m_renderer->DrawSprite(playerDesc);

	if (m_renderer != nullptr)
		m_renderer->ResetViewportRect();
	DrawUiShell();
	DrawInventory();
	DrawMinimap();
	DrawCraftingPanel();
	DrawMainActionPanel();
	DrawPlayerStatsPanel();
	DrawEquipmentTooltip();
	DrawPerformanceOverlay();
}

void GameEngine::DrawBlockCracks()
{
	const float viewHalfWidth = GetViewHalfWidth();
	const float viewHalfHeight = GetViewHalfHeight();
	const int startX = (std::max)(0, WorldToTileX(m_cameraX - viewHalfWidth - m_tileSize));
	const int endX = (std::min)(m_blockWidth - 1, WorldToTileX(m_cameraX + viewHalfWidth + m_tileSize));
	const int startY = (std::max)(0, WorldToTileY(m_cameraY + viewHalfHeight + m_tileSize));
	const int endY = (std::min)(m_blockHeight - 1, WorldToTileY(m_cameraY - viewHalfHeight - m_tileSize));

	for (int y = startY; y <= endY; ++y)
	{
		for (int x = startX; x <= endX; ++x)
		{
			const int blockIndex = y * m_blockWidth + x;
			const BlockBreakState& breakState = m_world.blockBreaks[blockIndex];
			if (!breakState.active || m_world.blocks[blockIndex].visible == 0)
				continue;

			DrawBlockCrackPattern(x, y, breakState.progress);
		}
	}

	for (const RemoteBlockBreakState& remoteBreak : m_networkState.remoteBlockBreaks)
	{
		if (!remoteBreak.active)
			continue;
		if (remoteBreak.tileX < startX || remoteBreak.tileX > endX || remoteBreak.tileY < startY || remoteBreak.tileY > endY)
			continue;
		if (!IsTileInBounds(remoteBreak.tileX, remoteBreak.tileY))
			continue;

		const int blockIndex = remoteBreak.tileY * m_blockWidth + remoteBreak.tileX;
		if (m_world.blocks[blockIndex].visible == 0)
			continue;

		DrawBlockCrackPattern(remoteBreak.tileX, remoteBreak.tileY, remoteBreak.progress);
	}
}

void GameEngine::DrawBlockCrackPattern(int tileX, int tileY, float progress)
{
	progress = Clamp01(progress);
	const float centerX = m_worldOriginX + tileX * m_tileSize - m_cameraX;
	const float centerY = m_worldOriginY - tileY * m_tileSize - m_cameraY;
	const float thickness = 1.25f + progress * 1.75f;
	const float alpha = 0.35f + progress * 0.55f;

	for (const CrackSegment& segment : CrackSegments)
	{
		if (progress <= segment.appearAt)
			continue;

		const float segmentProgress = Clamp01((progress - segment.appearAt) / (segment.completeAt - segment.appearAt));
		const float startX = centerX + segment.startX * m_tileSize;
		const float startY = centerY + segment.startY * m_tileSize;
		const float fullEndX = centerX + segment.endX * m_tileSize;
		const float fullEndY = centerY + segment.endY * m_tileSize;
		const float endX = startX + (fullEndX - startX) * segmentProgress;
		const float endY = startY + (fullEndY - startY) * segmentProgress;

		DrawCrackLine(startX, startY, endX, endY, thickness, alpha);
	}
}

void GameEngine::DrawCrackLine(float startX, float startY, float endX, float endY, float thickness, float alpha)
{
	const float deltaX = endX - startX;
	const float deltaY = endY - startY;
	const float length = std::sqrt(deltaX * deltaX + deltaY * deltaY);
	if (length < 0.1f)
		return;

	SpriteDesc crackDesc;
	crackDesc.textureFile = nullptr;
	crackDesc.positionX = (startX + endX) * 0.5f;
	crackDesc.positionY = (startY + endY) * 0.5f;
	crackDesc.width = length;
	crackDesc.height = thickness;
	crackDesc.rotationRadians = std::atan2(deltaY, deltaX);
	crackDesc.colorR = 0.025f;
	crackDesc.colorG = 0.022f;
	crackDesc.colorB = 0.018f;
	crackDesc.colorA = alpha;
	crackDesc.depth = -0.65f;

	m_renderer->DrawSprite(crackDesc);
}

void GameEngine::DrawLeafParticles()
{
	for (const LeafParticleState& particle : m_leafParticles)
	{
		if (!particle.alive || particle.lifetime <= 0.0f)
			continue;

		const float progress = Clamp01(particle.age / particle.lifetime);
		const float alpha = (1.0f - progress) * (1.0f - progress);
		if (alpha <= 0.01f)
			continue;

		SpriteDesc leafDesc;
		leafDesc.textureFile = BlockAtlasTexture;
		leafDesc.positionX = particle.x - m_cameraX;
		leafDesc.positionY = particle.y - m_cameraY;
		leafDesc.width = particle.size * (1.0f - progress * 0.28f);
		leafDesc.height = leafDesc.width;
		leafDesc.atlasColumns = BlockAtlasColumns;
		leafDesc.atlasRows = BlockAtlasRows;
		leafDesc.tileIndex = GetBlockAtlasTileIndex(BlockLeaves);
		leafDesc.rotationRadians = particle.rotation;
		leafDesc.colorR = 0.70f + progress * 0.16f;
		leafDesc.colorG = 1.0f;
		leafDesc.colorB = 0.66f - progress * 0.12f;
		leafDesc.colorA = alpha;
		leafDesc.depth = -0.92f;
		m_renderer->DrawSprite(leafDesc);
	}
}

void GameEngine::DrawMonsters()
{
	const float viewHalfWidth = GetViewHalfWidth();
	const float viewHalfHeight = GetViewHalfHeight();
	const float cullMargin = m_tileSize * 4.0f;
	const collib::AABB visibleWorldArea = collib::MakeAABB(
		m_cameraX,
		m_cameraY,
		viewHalfWidth * 2.0f + cullMargin * 2.0f,
		viewHalfHeight * 2.0f + cullMargin * 2.0f);
	QueryMonstersInAABB(visibleWorldArea, m_monsterQueryScratch);

	for (int monsterIndex : m_monsterQueryScratch)
	{
		const MonsterState& monster = m_monsters[monsterIndex];
		if (!monster.alive)
			continue;

		const float drawX = monster.x - m_cameraX;
		const float drawY = monster.y - m_cameraY;
		if (drawX < -viewHalfWidth - cullMargin || drawX > viewHalfWidth + cullMargin ||
			drawY < -viewHalfHeight - cullMargin || drawY > viewHalfHeight + cullMargin)
			continue;

		const float hurt = Clamp01(monster.hurtTimer / 0.22f);
		const float walk = std::sin(monster.animationTime);
		const float bob = std::fabs(walk) * 0.8f;
		const float recoilX = -static_cast<float>(monster.facing) * hurt * 4.0f;
		const float baseY = drawY + bob * 0.35f;
		const int biome = std::clamp(static_cast<int>(monster.biome), 0, BackgroundBiomeCount - 1);
		const bool moving = monster.idleTimer <= 0.0f || monster.contactTimer > 0.0f;
		const MonsterVisualDesc& visual = MonsterVisuals[biome];
		const bool hurtFrame = hurt > 0.01f && visual.hurtTexture != nullptr;
		const WCHAR* monsterTexture = hurtFrame ? visual.hurtTexture : (moving ? visual.moveTexture : visual.idleTexture);
		const int monsterColumns = (std::max)(1, hurtFrame ? visual.hurtFrames : (moving ? visual.moveFrames : visual.idleFrames));
		const int frame = static_cast<int>(monster.animationTime) % monsterColumns;

		SpriteDesc shadowDesc;
		shadowDesc.textureFile = nullptr;
		shadowDesc.positionX = drawX;
		shadowDesc.positionY = drawY - MonsterCollisionHeight * 0.52f;
		shadowDesc.width = std::clamp(visual.drawSize * 0.22f, 20.0f, 34.0f);
		shadowDesc.height = 3.2f;
		shadowDesc.colorA = 0.24f;
		shadowDesc.depth = -0.86f;
		m_renderer->DrawSprite(shadowDesc);

		SpriteDesc monsterDesc;
		monsterDesc.textureFile = monsterTexture;
		monsterDesc.positionX = drawX + recoilX;
		monsterDesc.positionY = baseY + visual.yOffset;
		monsterDesc.width = visual.drawSize;
		monsterDesc.height = visual.drawSize;
		monsterDesc.atlasColumns = monsterColumns;
		monsterDesc.atlasRows = MonsterSpriteRows;
		monsterDesc.tileIndex = frame;
		monsterDesc.flipX = monster.facing < 0 ? 1 : 0;
		monsterDesc.colorR = 1.0f;
		monsterDesc.colorG = 1.0f - hurt * 0.48f;
		monsterDesc.colorB = 1.0f - hurt * 0.48f;
		monsterDesc.depth = -0.95f;
		m_renderer->DrawSprite(monsterDesc);

		if (hurt > 0.0f)
		{
			DrawSolidRect(drawX - static_cast<float>(monster.facing) * 7.0f, baseY + 10.0f,
				8.0f, 2.0f, 0.95f, 0.18f, 0.12f, 0.35f * hurt, -0.98f);
		}

		const float healthRatio = Clamp01(static_cast<float>(monster.health) / static_cast<float>((std::max)(1, monster.maxHealth)));
		const float barWidth = std::clamp(visual.drawSize * 0.28f, 20.0f, 34.0f);
		const float barY = baseY + visual.yOffset + std::clamp(visual.drawSize * 0.18f, 14.0f, 24.0f);
		DrawSolidRect(drawX, barY, barWidth, 2.4f, 0.02f, 0.02f, 0.02f, 0.80f, -0.96f);
		DrawSolidRect(drawX - (barWidth * (1.0f - healthRatio)) * 0.5f, barY,
			barWidth * healthRatio, 2.4f, 0.86f, 0.18f, 0.14f, 1.0f, -0.97f);
	}
}

void GameEngine::DrawDroppedItems()
{
	const float viewHalfWidth = GetViewHalfWidth();
	const float viewHalfHeight = GetViewHalfHeight();
	const float cullMargin = m_tileSize * 2.0f;
	const float itemSize = m_tileSize * 0.72f;

	for (const DroppedItemState& item : m_droppedItems)
	{
		if (!item.alive)
			continue;

		const float drawX = item.x - m_cameraX;
		const float drawY = item.y - m_cameraY;
		if (drawX < -viewHalfWidth - cullMargin || drawX > viewHalfWidth + cullMargin ||
			drawY < -viewHalfHeight - cullMargin || drawY > viewHalfHeight + cullMargin)
			continue;

		SpriteDesc shadowDesc;
		shadowDesc.textureFile = nullptr;
		shadowDesc.positionX = drawX;
		shadowDesc.positionY = drawY - itemSize * 0.44f;
		shadowDesc.width = itemSize * 0.84f;
		shadowDesc.height = itemSize * 0.20f;
		shadowDesc.colorR = 0.0f;
		shadowDesc.colorG = 0.0f;
		shadowDesc.colorB = 0.0f;
		shadowDesc.colorA = 0.22f;
		shadowDesc.depth = -0.78f;
		m_renderer->DrawSprite(shadowDesc);
	}

	for (const DroppedItemState& item : m_droppedItems)
	{
		if (!item.alive)
			continue;

		const float drawX = item.x - m_cameraX;
		const float drawY = item.y - m_cameraY;
		if (drawX < -viewHalfWidth - cullMargin || drawX > viewHalfWidth + cullMargin ||
			drawY < -viewHalfHeight - cullMargin || drawY > viewHalfHeight + cullMargin)
			continue;

		SpriteDesc itemDesc;
		itemDesc.textureFile = BlockAtlasTexture;
		itemDesc.positionX = drawX;
		itemDesc.positionY = drawY;
		itemDesc.width = itemSize;
		itemDesc.height = itemSize;
		itemDesc.atlasColumns = BlockAtlasColumns;
		itemDesc.atlasRows = BlockAtlasRows;
		itemDesc.tileIndex = GetBlockAtlasTileIndex(GetDroppedItemTileIndex(item.tileIndex));
		itemDesc.depth = -0.90f;
		m_renderer->DrawSprite(itemDesc);
	}
}

void GameEngine::DrawProjectiles()
{
	const float viewHalfWidth = GetViewHalfWidth();
	const float viewHalfHeight = GetViewHalfHeight();
	const float cullMargin = m_tileSize * 2.0f;
	for (const ProjectileState& projectile : m_projectiles)
	{
		if (!projectile.alive)
			continue;

		const float drawX = projectile.x - m_cameraX;
		const float drawY = projectile.y - m_cameraY;
		if (drawX < -viewHalfWidth - cullMargin || drawX > viewHalfWidth + cullMargin ||
			drawY < -viewHalfHeight - cullMargin || drawY > viewHalfHeight + cullMargin)
			continue;

		const float angle = std::atan2(projectile.velocityY, projectile.velocityX);
		if (projectile.item == SlotBow)
		{
			SpriteDesc shaftDesc;
			shaftDesc.textureFile = nullptr;
			shaftDesc.positionX = drawX;
			shaftDesc.positionY = drawY;
			shaftDesc.width = 18.0f;
			shaftDesc.height = 2.0f;
			shaftDesc.rotationRadians = angle;
			shaftDesc.colorR = 0.70f;
			shaftDesc.colorG = 0.48f;
			shaftDesc.colorB = 0.22f;
			shaftDesc.colorA = 1.0f;
			shaftDesc.depth = -1.72f;
			m_renderer->DrawSprite(shaftDesc);

			SpriteDesc headDesc = shaftDesc;
			headDesc.positionX = drawX + std::cos(angle) * 8.0f;
			headDesc.positionY = drawY + std::sin(angle) * 8.0f;
			headDesc.width = 5.0f;
			headDesc.height = 3.0f;
			headDesc.colorR = 0.86f;
			headDesc.colorG = 0.90f;
			headDesc.colorB = 0.88f;
			headDesc.depth = -1.78f;
			m_renderer->DrawSprite(headDesc);
			continue;
		}

		SpriteDesc trailDesc;
		trailDesc.textureFile = nullptr;
		trailDesc.positionX = drawX - std::cos(angle) * 4.0f;
		trailDesc.positionY = drawY - std::sin(angle) * 4.0f;
		trailDesc.width = 13.0f;
		trailDesc.height = 2.4f;
		trailDesc.rotationRadians = angle;
		trailDesc.colorR = 1.0f;
		trailDesc.colorG = 0.86f;
		trailDesc.colorB = 0.34f;
		trailDesc.colorA = 0.88f;
		trailDesc.depth = -1.72f;
		m_renderer->DrawSprite(trailDesc);

		SpriteDesc coreDesc = trailDesc;
		coreDesc.positionX = drawX + std::cos(angle) * 3.0f;
		coreDesc.positionY = drawY + std::sin(angle) * 3.0f;
		coreDesc.width = 5.0f;
		coreDesc.height = 3.2f;
		coreDesc.colorR = 1.0f;
		coreDesc.colorG = 0.98f;
		coreDesc.colorB = 0.82f;
		coreDesc.colorA = 1.0f;
		coreDesc.depth = -1.80f;
		m_renderer->DrawSprite(coreDesc);
	}
}

void GameEngine::DrawRemotePlayers()
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer)
		return;

	const float viewHalfWidth = GetViewHalfWidth();
	const float viewHalfHeight = GetViewHalfHeight();
	const float cullMargin = m_tileSize * 4.0f;
	const float playerSpriteYOffset =
		-m_playerCollisionHeight * 0.5f -
		m_playerDrawHeight * 0.5f +
		(PlayerSpriteFootPixelY / PlayerSpriteFramePixelHeight) * m_playerDrawHeight;

	for (const RemotePlayerState& remotePlayer : m_networkState.remotePlayers)
	{
		if (!remotePlayer.active)
			continue;

		const PlayerState& player = remotePlayer.player;
		const float drawX = player.x - m_cameraX;
		const float drawY = player.y - m_cameraY;
		if (drawX < -viewHalfWidth - cullMargin || drawX > viewHalfWidth + cullMargin ||
			drawY < -viewHalfHeight - cullMargin || drawY > viewHalfHeight + cullMargin)
			continue;

		SpriteDesc shadowDesc;
		shadowDesc.textureFile = nullptr;
		shadowDesc.positionX = drawX;
		shadowDesc.positionY = drawY - m_playerCollisionHeight * 0.52f;
		shadowDesc.width = 20.0f;
		shadowDesc.height = 3.4f;
		shadowDesc.colorA = 0.24f;
		shadowDesc.depth = -0.82f;
		m_renderer->DrawSprite(shadowDesc);

		SpriteDesc playerDesc;
		playerDesc.positionX = drawX;
		playerDesc.positionY = drawY + playerSpriteYOffset;
		playerDesc.width = m_playerDrawWidth;
		playerDesc.height = m_playerDrawHeight;
		playerDesc.flipX = player.facing < 0 ? 1 : 0;
		playerDesc.depth = -1.05f;
		playerDesc.atlasRows = PlayerSpriteRows;
		playerDesc.colorR = 0.72f;
		playerDesc.colorG = 0.90f;
		playerDesc.colorB = 1.0f;

		const WCHAR* playerTexture = PlayerIdleTexture;
		int playerColumns = PlayerIdleFrames;
		int playerFrame = static_cast<int>(player.animationTime / 0.24f) % PlayerIdleFrames;
		if (remotePlayer.attackTimer > 0.0f)
		{
			const float attackProgress = 1.0f - Clamp01(remotePlayer.attackTimer / PlayerAttackDuration);
			const float lunge = std::sin(attackProgress * 3.1415926f);
			playerDesc.positionX += player.facing * (3.0f + lunge * 4.5f);
			playerDesc.positionY -= lunge * 1.4f;
			playerDesc.rotationRadians = player.facing * (0.035f + lunge * 0.060f);
			playerTexture = PlayerAttackTexture;
			playerColumns = PlayerAttackFrames;
			playerFrame = std::clamp(static_cast<int>(attackProgress * static_cast<float>(PlayerAttackFrames)), 0, PlayerAttackFrames - 1);
		}
		else if (!player.onGround)
		{
			if (player.velocityY < -8.0f)
			{
				playerTexture = PlayerFallTexture;
				playerColumns = PlayerFallFrames;
				playerFrame = static_cast<int>(player.animationTime / 0.13f) % PlayerFallFrames;
			}
			else
			{
				playerTexture = PlayerJumpTexture;
				playerColumns = PlayerJumpFrames;
				playerFrame = static_cast<int>(player.animationTime / 0.13f) % PlayerJumpFrames;
			}
		}
		else if (std::fabs(player.velocityX) > 1.0f)
		{
			playerTexture = PlayerRunTexture;
			playerColumns = PlayerRunFrames;
			playerFrame = static_cast<int>(player.animationTime / 0.11f) % PlayerRunFrames;
		}
		playerDesc.textureFile = playerTexture;
		playerDesc.atlasColumns = playerColumns;
		playerDesc.tileIndex = playerFrame;

		m_renderer->DrawSprite(playerDesc);

		const float healthRatio = Clamp01(static_cast<float>(remotePlayer.health) / static_cast<float>(GetPlayerMaxHealth()));
		const float barY = drawY + 25.0f;
		DrawSolidRect(drawX, barY, 22.0f, 3.0f, 0.02f, 0.02f, 0.02f, 0.68f, -1.12f);
		DrawSolidRect(drawX - 11.0f + 11.0f * healthRatio, barY,
			22.0f * healthRatio, 2.0f, 0.18f, 0.70f, 0.95f, 1.0f, -1.18f);

		char fallbackLabel[16] = {};
		const char* label = remotePlayer.nickname[0] != '\0' ? remotePlayer.nickname.data() : nullptr;
		if (label == nullptr)
		{
			std::snprintf(fallbackLabel, sizeof(fallbackLabel), "P%d", remotePlayer.id);
			label = fallbackLabel;
		}
		const float labelSize = 1.25f;
		const float labelX = drawX - GetUiTextWidth(label, labelSize) * 0.5f;
		DrawText(labelX + 1.0f, drawY + 37.0f, label, labelSize, 0.02f, 0.02f, 0.02f, 0.75f, -1.20f);
		DrawText(labelX, drawY + 38.0f, label, labelSize, 1.0f, 1.0f, 1.0f, 0.92f, -1.24f);
	}
}

void GameEngine::DrawPlayerAttackMotion()
{
	if (m_attackTimer <= 0.0f)
		return;

	const float attackProgress = 1.0f - Clamp01(m_attackTimer / PlayerAttackDuration);
	const float windup = std::sin(attackProgress * 3.1415926f);
	const float snap = std::sin(attackProgress * 6.2831853f);
	const float face = static_cast<float>(m_player.facing);
	const int selectedItem = GetSelectedInventoryItem();
	const int selectedSlot = m_inventory.GetSelectedSlot();
	const bool swordEquipped = selectedItem == SlotSword && m_inventory.GetSlotCount(selectedSlot) > 0;
	const bool axeEquipped = selectedItem == SlotAxe && m_inventory.GetSlotCount(selectedSlot) > 0;
	const float handX = m_player.x - m_cameraX + face * (12.0f + windup * 11.0f);
	const float handY = m_player.y - m_cameraY + 9.0f - windup * 1.8f;

	SpriteDesc armDesc;
	armDesc.textureFile = nullptr;
	armDesc.positionX = handX - face * 4.0f;
	armDesc.positionY = handY - 1.0f;
	armDesc.width = 17.0f;
	armDesc.height = 4.4f;
	armDesc.rotationRadians = face * (-0.45f + attackProgress * 1.45f);
	armDesc.colorR = 0.96f;
	armDesc.colorG = 0.76f;
	armDesc.colorB = 0.55f;
	armDesc.colorA = 0.96f;
	armDesc.depth = -1.62f;
	m_renderer->DrawSprite(armDesc);

	SpriteDesc handDesc;
	handDesc.textureFile = nullptr;
	handDesc.positionX = handX + face * 5.0f;
	handDesc.positionY = handY - 0.5f;
	handDesc.width = swordEquipped || axeEquipped ? 5.2f : 7.2f;
	handDesc.height = swordEquipped || axeEquipped ? 5.2f : 6.4f;
	handDesc.colorR = 0.98f;
	handDesc.colorG = 0.78f;
	handDesc.colorB = 0.58f;
	handDesc.colorA = 1.0f;
	handDesc.depth = -1.67f;
	m_renderer->DrawSprite(handDesc);

	if (swordEquipped)
	{
		SpriteDesc bladeDesc;
		bladeDesc.textureFile = nullptr;
		bladeDesc.positionX = handX + face * (19.0f + windup * 4.0f);
		bladeDesc.positionY = handY + 2.0f + snap * 2.2f;
		bladeDesc.width = 33.0f;
		bladeDesc.height = 5.2f;
		bladeDesc.rotationRadians = face * (-0.72f + attackProgress * 1.95f);
		bladeDesc.colorR = 0.82f;
		bladeDesc.colorG = 0.91f;
		bladeDesc.colorB = 0.95f;
		bladeDesc.colorA = 1.0f;
		bladeDesc.depth = -1.72f;
		m_renderer->DrawSprite(bladeDesc);

		SpriteDesc guardDesc = bladeDesc;
		guardDesc.positionX = handX + face * 7.8f;
		guardDesc.positionY = handY - 0.5f;
		guardDesc.width = 9.0f;
		guardDesc.height = 4.0f;
		guardDesc.rotationRadians += face * 1.55f;
		guardDesc.colorR = 0.83f;
		guardDesc.colorG = 0.62f;
		guardDesc.colorB = 0.26f;
		guardDesc.depth = -1.73f;
		m_renderer->DrawSprite(guardDesc);
		return;
	}

	if (axeEquipped)
	{
		SpriteDesc handleDesc;
		handleDesc.textureFile = nullptr;
		handleDesc.positionX = handX + face * (14.0f + windup * 3.0f);
		handleDesc.positionY = handY + 2.0f + snap * 1.5f;
		handleDesc.width = 28.0f;
		handleDesc.height = 4.8f;
		handleDesc.rotationRadians = face * (-1.05f + attackProgress * 2.25f);
		handleDesc.colorR = 0.38f;
		handleDesc.colorG = 0.21f;
		handleDesc.colorB = 0.09f;
		handleDesc.colorA = 1.0f;
		handleDesc.depth = -1.70f;
		m_renderer->DrawSprite(handleDesc);

		SpriteDesc headDesc = handleDesc;
		headDesc.positionX = handX + face * (26.0f + windup * 3.5f);
		headDesc.positionY = handY + 7.0f + snap * 1.8f;
		headDesc.width = 12.5f;
		headDesc.height = 10.0f;
		headDesc.rotationRadians += face * 0.20f;
		headDesc.colorR = 0.72f;
		headDesc.colorG = 0.78f;
		headDesc.colorB = 0.78f;
		headDesc.depth = -1.74f;
		m_renderer->DrawSprite(headDesc);
		return;
	}

	SpriteDesc punchDesc;
	punchDesc.textureFile = nullptr;
	punchDesc.positionX = handX + face * (10.0f + windup * 5.0f);
	punchDesc.positionY = handY;
	punchDesc.width = 9.0f;
	punchDesc.height = 7.0f;
	punchDesc.rotationRadians = face * 0.20f;
	punchDesc.colorR = 0.98f;
	punchDesc.colorG = 0.73f;
	punchDesc.colorB = 0.50f;
	punchDesc.colorA = 1.0f;
	punchDesc.depth = -1.72f;
	m_renderer->DrawSprite(punchDesc);
}

void GameEngine::DrawAttackArc()
{
	if (m_attackTimer <= 0.0f)
		return;

	const float progress = Clamp01(m_attackTimer / PlayerAttackDuration);
	const float centerX = m_player.x - m_cameraX + m_player.facing * (PlayerAttackRange * 0.58f);
	const float centerY = m_player.y - m_cameraY + 4.0f;

	SpriteDesc slashDesc;
	slashDesc.textureFile = nullptr;
	slashDesc.positionX = centerX;
	slashDesc.positionY = centerY;
	slashDesc.width = PlayerAttackRange * 0.92f;
	slashDesc.height = 4.5f;
	slashDesc.rotationRadians = m_player.facing > 0 ? -0.55f : 0.55f;
	slashDesc.colorR = 0.95f;
	slashDesc.colorG = 0.92f;
	slashDesc.colorB = 0.72f;
	slashDesc.colorA = 0.35f + progress * 0.55f;
	slashDesc.depth = -1.55f;
	m_renderer->DrawSprite(slashDesc);
}

void GameEngine::DrawInventory()
{
	const UiRect panel = GetInventoryPanelRect();
	DrawUiPanel(panel, "inventory");

	const float gridLeft = panel.left + UiTerminalListPad;
	const float gridRight = panel.left + panel.width - UiTerminalListPad;
	const float gridTop = panel.top - UiTerminalListTopGap;
	const float gridBottom = panel.top - panel.height + UiTerminalListBottomPad + 22.0f;
	const float gridWidth = (std::max)(24.0f, gridRight - gridLeft);
	const float gridHeight = (std::max)(24.0f, gridTop - gridBottom);
	const float cellWidth = gridWidth / static_cast<float>(InventoryVisibleColumns);
	const float cellHeight = gridHeight / static_cast<float>(InventoryVisibleRows);
	const float slotIconSize = (std::min)(cellWidth, cellHeight) * 0.54f;

	const int pageCount = InventoryPageCount;
	const int selectedSlot = m_inventory.GetSelectedSlot();
	const int dragSlot = m_inventory.GetDragSlot();
	const int currentPage = std::clamp(selectedSlot / InventoryVisibleSlotCount, 0, (std::max)(0, pageCount - 1));
	const int firstSlot = currentPage * InventoryVisibleSlotCount;
	const int lastSlot = (std::min)(InventorySlotCount, firstSlot + InventoryVisibleSlotCount);
	float cursorX = 0.0f;
	float cursorY = 0.0f;
	const int hoveredSlot = GetCursorViewPosition(cursorX, cursorY) ? GetInventorySlotAt(cursorX, cursorY) : -1;

	for (int visibleSlot = 0; visibleSlot < InventoryVisibleSlotCount; ++visibleSlot)
	{
		const int slot = firstSlot + visibleSlot;
		const int column = visibleSlot % InventoryVisibleColumns;
		const int row = visibleSlot / InventoryVisibleColumns;
		const float slotX = gridLeft + (static_cast<float>(column) + 0.5f) * cellWidth;
		const float slotY = gridTop - (static_cast<float>(row) + 0.5f) * cellHeight;
		const bool selected = slot == selectedSlot;
		const bool hovered = slot == hoveredSlot;
		const bool validSlot = slot < lastSlot;
		const int item = validSlot ? GetInventorySlotItem(slot) : InventoryEmptyItem;
		const int count = validSlot ? m_inventory.GetSlotCount(slot) : 0;
		const float iconAlpha = slot == dragSlot ? 0.34f : 1.0f;

		DrawSolidRect(slotX, slotY, cellWidth - 4.0f, cellHeight - 4.0f,
			UiThemeBodyDarkR, UiThemeBodyDarkG, UiThemeBodyDarkB, validSlot ? 0.56f : 0.30f, -6.95f);
		RectOutlineDesc cellOutline;
		cellOutline.positionX = slotX;
		cellOutline.positionY = slotY;
		cellOutline.width = cellWidth - 4.0f;
		cellOutline.height = cellHeight - 4.0f;
		cellOutline.thickness = 1.0f;
		cellOutline.colorR = UiThemeMutedR;
		cellOutline.colorG = UiThemeMutedG;
		cellOutline.colorB = UiThemeMutedB;
		cellOutline.colorA = validSlot ? 0.34f : 0.16f;
		cellOutline.depth = -7.15f;
		m_renderer->DrawRectOutline(cellOutline);

		if (hovered)
		{
			DrawSolidRect(slotX, slotY, cellWidth - 4.0f, cellHeight - 4.0f,
				0.06f, 0.26f, 0.22f, 0.48f, -7.02f);
		}

		if (selected)
		{
			RectOutlineDesc selectionDesc;
			selectionDesc.positionX = slotX;
			selectionDesc.positionY = slotY;
			selectionDesc.width = cellWidth - 5.0f;
			selectionDesc.height = cellHeight - 5.0f;
			selectionDesc.thickness = 2.0f;
			selectionDesc.colorR = UiThemeAmberR;
			selectionDesc.colorG = UiThemeAmberG;
			selectionDesc.colorB = UiThemeAmberB;
			selectionDesc.colorA = 0.98f;
			selectionDesc.depth = -7.05f;
			m_renderer->DrawRectOutline(selectionDesc);

			DrawSolidRect(slotX - cellWidth * 0.5f + 5.0f, slotY, 3.0f, cellHeight - 10.0f,
				UiThemeAmberR, UiThemeAmberG, UiThemeAmberB, 0.92f, -7.2f);
		}

		if (item != InventoryEmptyItem)
		{
			DrawInventoryItemIcon(item, slotX, slotY + cellHeight * 0.18f, slotIconSize, -7.4f, iconAlpha);
		}

		if (count > 0)
		{
			char countText[8] = {};
			std::snprintf(countText, sizeof(countText), "%d", count);
			DrawUiText(slotX - GetUiTextWidth(countText, 1.10f) * 0.5f, slotY - cellHeight * 0.25f, countText, 1.10f,
				UiThemeCreamR, UiThemeCreamG, UiThemeCreamB, slot == dragSlot ? 0.42f : 1.0f, -8.0f);
		}
	}

	if (dragSlot >= 0 && hoveredSlot >= 0 && IsKeyHeld(VK_LBUTTON))
	{
		const int draggedItem = GetInventorySlotItem(dragSlot);
		if (draggedItem != InventoryEmptyItem)
			DrawInventoryItemIcon(draggedItem, cursorX, cursorY, slotIconSize, -12.6f, 0.92f);
	}

	char pageText[16] = {};
	std::snprintf(pageText, sizeof(pageText), "%d/%d", currentPage + 1, pageCount);
	DrawUiText(panel.left + UiTerminalListPad, panel.top - panel.height + 13.0f, pageText, 1.10f,
		UiThemeCreamR, UiThemeCreamG, UiThemeCreamB, 1.0f, -8.0f);
}

void GameEngine::DrawWeaponIcon(float centerX, float centerY, float size, float depth, float alpha, int slot)
{
	if (slot == SlotIronHelmet || slot == SlotIronArmor || slot == SlotSwiftBoots || slot == SlotLuckyCharm)
	{
		const InventoryItemInfo& item = GetInventoryItemInfo(slot);
		SpriteDesc baseDesc;
		baseDesc.textureFile = nullptr;
		baseDesc.positionX = centerX;
		baseDesc.positionY = centerY;
		baseDesc.width = size * (slot == SlotIronArmor ? 0.62f : 0.54f);
		baseDesc.height = size * (slot == SlotSwiftBoots ? 0.34f : 0.50f);
		baseDesc.colorR = slot == SlotLuckyCharm ? 0.88f : item.iconR;
		baseDesc.colorG = slot == SlotLuckyCharm ? 0.74f : item.iconG;
		baseDesc.colorB = slot == SlotLuckyCharm ? 0.26f : item.iconB;
		baseDesc.colorA = alpha;
		baseDesc.depth = depth;
		if (slot == SlotIronHelmet)
		{
			baseDesc.positionY = centerY + size * 0.03f;
			baseDesc.height = size * 0.42f;
		}
		else if (slot == SlotSwiftBoots)
		{
			baseDesc.rotationRadians = -0.18f;
		}
		else if (slot == SlotLuckyCharm)
		{
			baseDesc.width = size * 0.46f;
			baseDesc.height = size * 0.46f;
			baseDesc.rotationRadians = 0.78f;
		}
		m_renderer->DrawSprite(baseDesc);

		SpriteDesc shineDesc = baseDesc;
		shineDesc.positionX -= size * 0.13f;
		shineDesc.positionY += size * 0.12f;
		shineDesc.width = size * 0.09f;
		shineDesc.height = size * 0.24f;
		shineDesc.colorR = 0.96f;
		shineDesc.colorG = 0.98f;
		shineDesc.colorB = 0.92f;
		shineDesc.colorA = alpha * 0.72f;
		shineDesc.depth = depth - 0.10f;
		m_renderer->DrawSprite(shineDesc);
		return;
	}

	if (slot == SlotPistol)
	{
		SpriteDesc barrelDesc;
		barrelDesc.textureFile = nullptr;
		barrelDesc.positionX = centerX + size * 0.10f;
		barrelDesc.positionY = centerY + size * 0.06f;
		barrelDesc.width = size * 0.62f;
		barrelDesc.height = size * 0.18f;
		barrelDesc.colorR = 0.40f;
		barrelDesc.colorG = 0.46f;
		barrelDesc.colorB = 0.50f;
		barrelDesc.colorA = alpha;
		barrelDesc.depth = depth;
		m_renderer->DrawSprite(barrelDesc);

		SpriteDesc gripDesc = barrelDesc;
		gripDesc.positionX = centerX - size * 0.12f;
		gripDesc.positionY = centerY - size * 0.19f;
		gripDesc.width = size * 0.18f;
		gripDesc.height = size * 0.44f;
		gripDesc.rotationRadians = -0.32f;
		gripDesc.colorR = 0.24f;
		gripDesc.colorG = 0.16f;
		gripDesc.colorB = 0.10f;
		gripDesc.depth = depth - 0.05f;
		m_renderer->DrawSprite(gripDesc);
		return;
	}

	if (slot == SlotBow)
	{
		SpriteDesc bowDesc;
		bowDesc.textureFile = nullptr;
		bowDesc.positionX = centerX;
		bowDesc.positionY = centerY;
		bowDesc.width = size * 0.16f;
		bowDesc.height = size * 0.82f;
		bowDesc.rotationRadians = -0.28f;
		bowDesc.colorR = 0.58f;
		bowDesc.colorG = 0.32f;
		bowDesc.colorB = 0.12f;
		bowDesc.colorA = alpha;
		bowDesc.depth = depth;
		m_renderer->DrawSprite(bowDesc);

		SpriteDesc stringDesc = bowDesc;
		stringDesc.positionX = centerX + size * 0.18f;
		stringDesc.width = size * 0.04f;
		stringDesc.height = size * 0.76f;
		stringDesc.colorR = 0.88f;
		stringDesc.colorG = 0.86f;
		stringDesc.colorB = 0.76f;
		stringDesc.depth = depth - 0.08f;
		m_renderer->DrawSprite(stringDesc);
		return;
	}

	if (slot == SlotAxe)
	{
		SpriteDesc handleDesc;
		handleDesc.textureFile = nullptr;
		handleDesc.positionX = centerX - size * 0.05f;
		handleDesc.positionY = centerY - size * 0.06f;
		handleDesc.width = size * 0.16f;
		handleDesc.height = size * 0.88f;
		handleDesc.rotationRadians = -0.62f;
		handleDesc.colorR = 0.38f;
		handleDesc.colorG = 0.21f;
		handleDesc.colorB = 0.09f;
		handleDesc.colorA = alpha;
		handleDesc.depth = depth;
		m_renderer->DrawSprite(handleDesc);

		SpriteDesc headDesc;
		headDesc.textureFile = nullptr;
		headDesc.positionX = centerX + size * 0.20f;
		headDesc.positionY = centerY + size * 0.24f;
		headDesc.width = size * 0.52f;
		headDesc.height = size * 0.32f;
		headDesc.rotationRadians = -0.20f;
		headDesc.colorR = 0.66f;
		headDesc.colorG = 0.72f;
		headDesc.colorB = 0.74f;
		headDesc.colorA = alpha;
		headDesc.depth = depth - 0.05f;
		m_renderer->DrawSprite(headDesc);

		SpriteDesc edgeDesc;
		edgeDesc.textureFile = nullptr;
		edgeDesc.positionX = centerX + size * 0.37f;
		edgeDesc.positionY = centerY + size * 0.24f;
		edgeDesc.width = size * 0.12f;
		edgeDesc.height = size * 0.35f;
		edgeDesc.rotationRadians = -0.20f;
		edgeDesc.colorR = 0.88f;
		edgeDesc.colorG = 0.92f;
		edgeDesc.colorB = 0.90f;
		edgeDesc.colorA = alpha;
		edgeDesc.depth = depth - 0.10f;
		m_renderer->DrawSprite(edgeDesc);
		return;
	}

	SpriteDesc bladeDesc;
	bladeDesc.textureFile = nullptr;
	bladeDesc.positionX = centerX + size * 0.05f;
	bladeDesc.positionY = centerY + size * 0.05f;
	bladeDesc.width = size * 0.72f;
	bladeDesc.height = size * 0.16f;
	bladeDesc.rotationRadians = 0.78f;
	bladeDesc.colorR = 0.76f;
	bladeDesc.colorG = 0.83f;
	bladeDesc.colorB = 0.86f;
	bladeDesc.colorA = alpha;
	bladeDesc.depth = depth;
	m_renderer->DrawSprite(bladeDesc);

	SpriteDesc guardDesc;
	guardDesc.textureFile = nullptr;
	guardDesc.positionX = centerX - size * 0.20f;
	guardDesc.positionY = centerY - size * 0.18f;
	guardDesc.width = size * 0.40f;
	guardDesc.height = size * 0.12f;
	guardDesc.rotationRadians = 0.78f;
	guardDesc.colorR = 0.78f;
	guardDesc.colorG = 0.58f;
	guardDesc.colorB = 0.24f;
	guardDesc.colorA = alpha;
	guardDesc.depth = depth - 0.05f;
	m_renderer->DrawSprite(guardDesc);

	SpriteDesc handleDesc;
	handleDesc.textureFile = nullptr;
	handleDesc.positionX = centerX - size * 0.31f;
	handleDesc.positionY = centerY - size * 0.29f;
	handleDesc.width = size * 0.26f;
	handleDesc.height = size * 0.13f;
	handleDesc.rotationRadians = 0.78f;
	handleDesc.colorR = 0.30f;
	handleDesc.colorG = 0.17f;
	handleDesc.colorB = 0.08f;
	handleDesc.colorA = alpha;
	handleDesc.depth = depth - 0.1f;
	m_renderer->DrawSprite(handleDesc);
}

void GameEngine::DrawPotionIcon(float centerX, float centerY, float size, float depth, float alpha, int item)
{
	const InventoryItemInfo& info = GetInventoryItemInfo(item);
	SpriteDesc corkDesc;
	corkDesc.textureFile = nullptr;
	corkDesc.positionX = centerX;
	corkDesc.positionY = centerY + size * 0.33f;
	corkDesc.width = size * 0.22f;
	corkDesc.height = size * 0.16f;
	corkDesc.colorR = 0.50f;
	corkDesc.colorG = 0.28f;
	corkDesc.colorB = 0.12f;
	corkDesc.colorA = alpha;
	corkDesc.depth = depth - 0.08f;
	m_renderer->DrawSprite(corkDesc);

	SpriteDesc bodyDesc = corkDesc;
	bodyDesc.positionY = centerY - size * 0.07f;
	bodyDesc.width = size * 0.58f;
	bodyDesc.height = size * 0.58f;
	bodyDesc.colorR = info.iconR;
	bodyDesc.colorG = info.iconG;
	bodyDesc.colorB = info.iconB;
	bodyDesc.colorA = alpha * 0.86f;
	bodyDesc.depth = depth - 0.04f;
	m_renderer->DrawSprite(bodyDesc);

	SpriteDesc neckDesc = bodyDesc;
	neckDesc.positionY = centerY + size * 0.18f;
	neckDesc.width = size * 0.28f;
	neckDesc.height = size * 0.22f;
	neckDesc.colorR = info.iconR + 0.10f;
	neckDesc.colorG = info.iconG + 0.10f;
	neckDesc.colorB = info.iconB + 0.10f;
	neckDesc.colorA = alpha * 0.72f;
	neckDesc.depth = depth - 0.06f;
	m_renderer->DrawSprite(neckDesc);

	SpriteDesc shineDesc = bodyDesc;
	shineDesc.positionX = centerX - size * 0.15f;
	shineDesc.positionY = centerY + size * 0.02f;
	shineDesc.width = size * 0.08f;
	shineDesc.height = size * 0.28f;
	shineDesc.colorR = 0.92f;
	shineDesc.colorG = 1.0f;
	shineDesc.colorB = 1.0f;
	shineDesc.colorA = alpha * 0.68f;
	shineDesc.depth = depth - 0.12f;
	m_renderer->DrawSprite(shineDesc);
}

void GameEngine::DrawAmmoIcon(float centerX, float centerY, float size, float depth, float alpha, int item)
{
	if (item == SlotArrow)
	{
		SpriteDesc shaftDesc;
		shaftDesc.textureFile = nullptr;
		shaftDesc.positionX = centerX;
		shaftDesc.positionY = centerY;
		shaftDesc.width = size * 0.82f;
		shaftDesc.height = size * 0.07f;
		shaftDesc.rotationRadians = 0.35f;
		shaftDesc.colorR = 0.70f;
		shaftDesc.colorG = 0.50f;
		shaftDesc.colorB = 0.28f;
		shaftDesc.colorA = alpha;
		shaftDesc.depth = depth;
		m_renderer->DrawSprite(shaftDesc);

		SpriteDesc headDesc = shaftDesc;
		headDesc.positionX = centerX + size * 0.35f;
		headDesc.positionY = centerY + size * 0.13f;
		headDesc.width = size * 0.18f;
		headDesc.height = size * 0.14f;
		headDesc.colorR = 0.78f;
		headDesc.colorG = 0.82f;
		headDesc.colorB = 0.82f;
		headDesc.depth = depth - 0.08f;
		m_renderer->DrawSprite(headDesc);
		return;
	}

	SpriteDesc bulletDesc;
	bulletDesc.textureFile = nullptr;
	bulletDesc.positionX = centerX;
	bulletDesc.positionY = centerY;
	bulletDesc.width = size * 0.24f;
	bulletDesc.height = size * 0.56f;
	bulletDesc.colorR = 0.88f;
	bulletDesc.colorG = 0.72f;
	bulletDesc.colorB = 0.30f;
	bulletDesc.colorA = alpha;
	bulletDesc.depth = depth;
	m_renderer->DrawSprite(bulletDesc);
}

void GameEngine::DrawTeleportPotionIcon(float centerX, float centerY, float size, float depth, float alpha)
{
	SpriteDesc corkDesc;
	corkDesc.textureFile = nullptr;
	corkDesc.positionX = centerX;
	corkDesc.positionY = centerY + size * 0.33f;
	corkDesc.width = size * 0.22f;
	corkDesc.height = size * 0.16f;
	corkDesc.colorR = 0.50f;
	corkDesc.colorG = 0.28f;
	corkDesc.colorB = 0.12f;
	corkDesc.colorA = alpha;
	corkDesc.depth = depth - 0.08f;
	m_renderer->DrawSprite(corkDesc);

	SpriteDesc neckDesc = corkDesc;
	neckDesc.positionY = centerY + size * 0.18f;
	neckDesc.width = size * 0.28f;
	neckDesc.height = size * 0.22f;
	neckDesc.colorR = 0.52f;
	neckDesc.colorG = 0.88f;
	neckDesc.colorB = 0.96f;
	neckDesc.colorA = alpha * 0.78f;
	neckDesc.depth = depth - 0.06f;
	m_renderer->DrawSprite(neckDesc);

	SpriteDesc bodyDesc = neckDesc;
	bodyDesc.positionY = centerY - size * 0.08f;
	bodyDesc.width = size * 0.58f;
	bodyDesc.height = size * 0.48f;
	bodyDesc.colorR = 0.40f;
	bodyDesc.colorG = 0.78f;
	bodyDesc.colorB = 0.98f;
	bodyDesc.colorA = alpha * 0.82f;
	bodyDesc.depth = depth - 0.04f;
	m_renderer->DrawSprite(bodyDesc);

	SpriteDesc liquidDesc = bodyDesc;
	liquidDesc.positionY = centerY - size * 0.16f;
	liquidDesc.width = size * 0.50f;
	liquidDesc.height = size * 0.28f;
	liquidDesc.colorR = 0.44f;
	liquidDesc.colorG = 0.30f;
	liquidDesc.colorB = 0.96f;
	liquidDesc.colorA = alpha;
	liquidDesc.depth = depth - 0.10f;
	m_renderer->DrawSprite(liquidDesc);

	SpriteDesc shineDesc = bodyDesc;
	shineDesc.positionX = centerX - size * 0.15f;
	shineDesc.positionY = centerY + size * 0.02f;
	shineDesc.width = size * 0.08f;
	shineDesc.height = size * 0.28f;
	shineDesc.colorR = 0.92f;
	shineDesc.colorG = 1.0f;
	shineDesc.colorB = 1.0f;
	shineDesc.colorA = alpha * 0.68f;
	shineDesc.depth = depth - 0.12f;
	m_renderer->DrawSprite(shineDesc);
}

void GameEngine::DrawInventoryItemIcon(int item, float centerX, float centerY, float size, float depth, float alpha)
{
	const InventoryItemKind kind = GetInventoryItemKind(item);
	if (kind == InventoryItemKind::Equipment)
	{
		DrawWeaponIcon(centerX, centerY, size, depth, alpha, item);
		return;
	}

	if (kind == InventoryItemKind::Consumable)
	{
		DrawPotionIcon(centerX, centerY, size, depth, alpha, item);
		return;
	}

	if (kind == InventoryItemKind::Ammo)
	{
		DrawAmmoIcon(centerX, centerY, size, depth, alpha, item);
		return;
	}

	if ((kind == InventoryItemKind::TerrainBlock || kind == InventoryItemKind::Furniture) &&
		item >= 0 && item < BlockInventorySlotCount)
	{
		SpriteDesc blockDesc;
		blockDesc.textureFile = BlockAtlasTexture;
		blockDesc.positionX = centerX;
		blockDesc.positionY = centerY;
		blockDesc.width = size;
		blockDesc.height = size;
		blockDesc.atlasColumns = BlockAtlasColumns;
		blockDesc.atlasRows = BlockAtlasRows;
		blockDesc.tileIndex = GetBlockAtlasTileIndex(InventoryTileIndices[item]);
		blockDesc.colorA = alpha;
		blockDesc.depth = depth;
		m_renderer->DrawSprite(blockDesc);
	}
}

void GameEngine::DrawMinimap()
{
	{
		const bool expanded = m_minimapExpanded;
		UploadMinimapDirtyTiles();

		const UiRect panel = GetRightPanelRect(0);
		const float compactInsetX = 4.0f;
		const float compactInsetTop = 18.0f;
		const float compactInsetBottom = 4.0f;
		float mapLeft = panel.left + compactInsetX;
		float mapTop = panel.top - compactInsetTop;
		float mapWidth = (std::max)(1.0f, panel.width - compactInsetX * 2.0f);
		float mapHeight = (std::max)(1.0f, panel.height - compactInsetTop - compactInsetBottom);
		int sourceX = 0;
		int sourceY = 0;
		int sourceWidth = (std::max)(1, m_blockWidth);
		int sourceHeight = (std::max)(1, m_blockHeight);

		if (expanded)
		{
			DrawSolidRect(0.0f, 0.0f, GetViewHalfWidth() * 2.0f, GetViewHalfHeight() * 2.0f,
				0.0f, 0.0f, 0.0f, 0.42f, -18.0f);

			const float availableWidth = GetViewHalfWidth() * 2.0f - 88.0f;
			const float availableHeight = GetViewHalfHeight() * 2.0f - 102.0f;
			const float worldAspect = m_blockHeight > 0 ? static_cast<float>(m_blockWidth) / static_cast<float>(m_blockHeight) : 1.0f;
			mapWidth = availableWidth;
			mapHeight = mapWidth / (std::max)(0.01f, worldAspect);
			if (mapHeight > availableHeight)
			{
				mapHeight = availableHeight;
				mapWidth = mapHeight * worldAspect;
			}
			mapLeft = -mapWidth * 0.5f;
			mapTop = mapHeight * 0.5f;
			DrawSolidRect(0.0f, 0.0f, mapWidth + 8.0f, mapHeight + 8.0f,
				0.015f, 0.018f, 0.022f, 0.88f, -18.5f);
		}
		else
		{
			DrawClassicUiPanel(panel, "map");
			DrawSolidRect(mapLeft + mapWidth * 0.5f, mapTop - mapHeight * 0.5f,
				mapWidth, mapHeight, UiThemeShadowR, UiThemeShadowG, UiThemeShadowB, 0.86f, -6.25f);

			const float mapAspect = mapWidth / (std::max)(1.0f, mapHeight);
			const UiRect gameViewport = GetGameViewportRect();
			const int visibleTileColumns = (std::max)(1, static_cast<int>(std::ceil(gameViewport.width / m_tileSize)));
			const int visibleTileRows = (std::max)(1, static_cast<int>(std::ceil(gameViewport.height / m_tileSize)));
			const float compactWorldScale = 3.15f;
			sourceWidth = (std::max)(48, static_cast<int>(std::ceil(static_cast<float>(visibleTileColumns) * compactWorldScale)));
			sourceHeight = (std::max)(34, static_cast<int>(std::ceil(static_cast<float>(visibleTileRows) * compactWorldScale)));
			if (static_cast<float>(sourceWidth) / static_cast<float>(sourceHeight) > mapAspect)
				sourceHeight = (std::max)(34, static_cast<int>(std::ceil(static_cast<float>(sourceWidth) / mapAspect)));
			else
				sourceWidth = (std::max)(48, static_cast<int>(std::ceil(static_cast<float>(sourceHeight) * mapAspect)));
			sourceHeight = (std::min)(sourceHeight, m_blockHeight);
			sourceWidth = (std::min)(sourceWidth, m_blockWidth);
			const int centerTileX = std::clamp(WorldToTileX(m_cameraX), 0, (std::max)(0, m_blockWidth - 1));
			const int centerTileY = std::clamp(WorldToTileY(m_cameraY), 0, (std::max)(0, m_blockHeight - 1));
			sourceX = std::clamp(centerTileX - sourceWidth / 2, 0, (std::max)(0, m_blockWidth - sourceWidth));
			sourceY = std::clamp(centerTileY - sourceHeight / 2, 0, (std::max)(0, m_blockHeight - sourceHeight));
		}

		if (m_minimapTextureId != 0 && m_blockWidth > 0 && m_blockHeight > 0)
		{
			SpriteDesc mapDesc;
			mapDesc.positionX = mapLeft + mapWidth * 0.5f;
			mapDesc.positionY = mapTop - mapHeight * 0.5f;
			mapDesc.width = mapWidth;
			mapDesc.height = mapHeight;
			mapDesc.uvX = static_cast<float>(sourceX) / static_cast<float>(m_blockWidth);
			mapDesc.uvY = static_cast<float>(sourceY) / static_cast<float>(m_blockHeight);
			mapDesc.uvWidth = static_cast<float>(sourceWidth) / static_cast<float>(m_blockWidth);
			mapDesc.uvHeight = static_cast<float>(sourceHeight) / static_cast<float>(m_blockHeight);
			mapDesc.colorA = 1.0f;
			mapDesc.depth = expanded ? -19.0f : -6.6f;
			m_renderer->DrawDynamicTexture(m_minimapTextureId, mapDesc);
		}

		const float tileScreenSize = (std::min)(mapWidth / static_cast<float>((std::max)(1, sourceWidth)),
			mapHeight / static_cast<float>((std::max)(1, sourceHeight)));
		auto drawMapMarker = [&](float worldX, float worldY, float markerSize, float colorR, float colorG, float colorB, bool requireReveal)
		{
			const int tileX = WorldToTileX(worldX);
			const int tileY = WorldToTileY(worldY);
			if (tileX < sourceX || tileX >= sourceX + sourceWidth || tileY < sourceY || tileY >= sourceY + sourceHeight)
				return;
			if (requireReveal && !IsMapTileRevealed(tileX, tileY))
				return;

			const float normalizedX = (static_cast<float>(tileX - sourceX) + 0.5f) / static_cast<float>((std::max)(1, sourceWidth));
			const float normalizedY = (static_cast<float>(tileY - sourceY) + 0.5f) / static_cast<float>((std::max)(1, sourceHeight));
			DrawSolidRect(mapLeft + normalizedX * mapWidth, mapTop - normalizedY * mapHeight,
				markerSize, markerSize, colorR, colorG, colorB, 1.0f, expanded ? -19.4f : -7.2f);
		};

		const float playerMarkerSize = std::clamp(tileScreenSize * 1.6f, expanded ? 4.0f : 4.5f, expanded ? 8.0f : 7.0f);
		const float monsterMarkerSize = std::clamp(tileScreenSize * 1.20f, expanded ? 3.0f : 3.5f, expanded ? 6.0f : 5.5f);
		drawMapMarker(m_player.x, m_player.y, playerMarkerSize, 0.35f, 0.74f, 1.0f, false);

		const float mapWorldCenterX = m_worldOriginX + (static_cast<float>(sourceX) + static_cast<float>(sourceWidth) * 0.5f) * m_tileSize;
		const float mapWorldCenterY = m_worldOriginY - (static_cast<float>(sourceY) + static_cast<float>(sourceHeight) * 0.5f) * m_tileSize;
		const collib::AABB mapWorldArea = collib::MakeAABB(
			mapWorldCenterX,
			mapWorldCenterY,
			static_cast<float>(sourceWidth) * m_tileSize,
			static_cast<float>(sourceHeight) * m_tileSize);
		QueryMonstersInAABB(mapWorldArea, m_monsterQueryScratch);
		for (int monsterIndex : m_monsterQueryScratch)
		{
			const MonsterState& monster = m_monsters[monsterIndex];
			drawMapMarker(monster.x, monster.y, monsterMarkerSize, 1.0f, 0.22f, 0.18f, true);
		}

		if (expanded)
		{
			RectOutlineDesc outlineDesc;
			outlineDesc.positionX = mapLeft + mapWidth * 0.5f;
			outlineDesc.positionY = mapTop - mapHeight * 0.5f;
			outlineDesc.width = mapWidth + 8.0f;
			outlineDesc.height = mapHeight + 8.0f;
			outlineDesc.thickness = 1.5f;
			outlineDesc.colorR = UiThemeCreamR;
			outlineDesc.colorG = UiThemeCreamG;
			outlineDesc.colorB = UiThemeCreamB;
			outlineDesc.colorA = 0.8f;
			outlineDesc.depth = -19.8f;
			m_renderer->DrawRectOutline(outlineDesc);
		}

		return;
	}

	const bool expanded = m_minimapExpanded;
	const UiRect panel = GetRightPanelRect(0);
	const float compactMapInsetX = 6.0f;
	const float compactMapInsetTop = 20.0f;
	const float compactMapInsetBottom = 6.0f;
	const float compactMapWidth = (std::max)(1.0f, panel.width - compactMapInsetX * 2.0f);
	const float compactMapHeight = (std::max)(1.0f, panel.height - compactMapInsetTop - compactMapInsetBottom);
	const bool compactPanelHasRoom = compactMapWidth >= 126.0f && compactMapHeight >= 52.0f;
	const int sampleStep = expanded ? 1 : (compactPanelHasRoom ? 2 : 3);
	const int centerTileX = std::clamp(WorldToTileX(m_cameraX), 0, m_blockWidth - 1);
	const int centerTileY = std::clamp(WorldToTileY(m_cameraY), 0, m_blockHeight - 1);
	int startTileX = 0;
	int endTileX = m_blockWidth - 1;
	int startTileY = 0;
	int endTileY = m_blockHeight - 1;

	if (!expanded)
	{
		const float panelAspect = compactMapWidth / compactMapHeight;
		const int halfTilesY = 18;
		const int halfTilesX = (std::max)(24, static_cast<int>(static_cast<float>(halfTilesY) * panelAspect + 0.5f));
		startTileX = (std::max)(0, centerTileX - halfTilesX);
		endTileX = (std::min)(m_blockWidth - 1, centerTileX + halfTilesX);
		startTileY = (std::max)(0, centerTileY - halfTilesY);
		endTileY = (std::min)(m_blockHeight - 1, centerTileY + halfTilesY);
	}

	const int sampleWidth = ((endTileX - startTileX) / sampleStep) + 1;
	const int sampleHeight = ((endTileY - startTileY) / sampleStep) + 1;
	float pixelSize = 3.0f;
	if (expanded)
	{
		const float availableWidth = GetViewHalfWidth() * 2.0f - 88.0f;
		const float availableHeight = GetViewHalfHeight() * 2.0f - 102.0f;
		pixelSize = std::clamp((std::min)(availableWidth / static_cast<float>(sampleWidth), availableHeight / static_cast<float>(sampleHeight)),
			0.70f, 4.0f);
	}
	else
	{
		pixelSize = std::clamp((std::min)(compactMapWidth / static_cast<float>(sampleWidth), compactMapHeight / static_cast<float>(sampleHeight)),
			0.85f, 24.0f);
	}

	const float mapWidth = sampleWidth * pixelSize;
	const float mapHeight = sampleHeight * pixelSize;
	const float mapLeft = expanded ? -mapWidth * 0.5f : panel.left + compactMapInsetX + (compactMapWidth - mapWidth) * 0.5f;
	const float mapTop = expanded ? mapHeight * 0.5f : panel.top - compactMapInsetTop - (compactMapHeight - mapHeight) * 0.5f;
	const float overlayDepth = expanded ? -18.0f : -5.2f;
	const float mapPanelDepth = expanded ? -18.5f : -6.0f;
	const float mapTileDepth = expanded ? -19.0f : -6.5f;
	const float mapMarkerDepth = expanded ? -19.4f : -7.0f;
	const float mapOutlineDepth = expanded ? -19.8f : -7.2f;

	if (expanded)
	{
		DrawSolidRect(0.0f, 0.0f, GetViewHalfWidth() * 2.0f, GetViewHalfHeight() * 2.0f,
			0.0f, 0.0f, 0.0f, 0.42f, overlayDepth);
	}
	else
	{
		DrawClassicUiPanel(panel, "map");
		DrawSolidRect(panel.left + compactMapInsetX + compactMapWidth * 0.5f,
			panel.top - compactMapInsetTop - compactMapHeight * 0.5f,
			compactMapWidth,
			compactMapHeight,
			UiThemeShadowR, UiThemeShadowG, UiThemeShadowB, 0.72f, -6.25f);
	}

	if (expanded)
	{
		DrawSolidRect(mapLeft + mapWidth * 0.5f, mapTop - mapHeight * 0.5f, mapWidth + 8.0f, mapHeight + 8.0f,
			0.015f, 0.018f, 0.022f, 0.88f, mapPanelDepth);
	}

	auto getMapColor = [](unsigned short tileIndex, float& colorR, float& colorG, float& colorB)
	{
		colorR = 0.24f;
		colorG = 0.24f;
		colorB = 0.25f;
		switch (tileIndex)
		{
		case BlockGrass:
			colorR = 0.22f; colorG = 0.62f; colorB = 0.22f;
			break;
		case BlockDirt:
			colorR = 0.38f; colorG = 0.24f; colorB = 0.13f;
			break;
		case BlockStone:
			colorR = 0.38f; colorG = 0.40f; colorB = 0.42f;
			break;
		case BlockOre:
			colorR = 0.50f; colorG = 0.64f; colorB = 0.78f;
			break;
		case BlockSand:
			colorR = 0.82f; colorG = 0.68f; colorB = 0.32f;
			break;
		case BlockWood:
		case BlockPlacedWood:
			colorR = 0.44f; colorG = 0.25f; colorB = 0.11f;
			break;
		case BlockLeaves:
			colorR = 0.16f; colorG = 0.55f; colorB = 0.20f;
			break;
		case BlockCraftingTable:
			colorR = 0.78f; colorG = 0.46f; colorB = 0.18f;
			break;
		case BlockPrairieStone:
			colorR = 0.34f; colorG = 0.39f; colorB = 0.34f;
			break;
		case BlockMossStone:
			colorR = 0.26f; colorG = 0.44f; colorB = 0.25f;
			break;
		case BlockSandstone:
			colorR = 0.70f; colorG = 0.54f; colorB = 0.28f;
			break;
		case BlockDesertStone:
			colorR = 0.50f; colorG = 0.35f; colorB = 0.22f;
			break;
		case BlockSnow:
			colorR = 0.86f; colorG = 0.94f; colorB = 0.96f;
			break;
		case BlockIce:
			colorR = 0.44f; colorG = 0.72f; colorB = 0.88f;
			break;
		case BlockFrozenStone:
			colorR = 0.38f; colorG = 0.50f; colorB = 0.58f;
			break;
		case BlockCrystalOre:
			colorR = 0.36f; colorG = 0.88f; colorB = 0.96f;
			break;
		}

		return static_cast<unsigned int>(tileIndex) + 1u;
	};

	const bool rebuildMinimap =
		m_minimapDirty ||
		m_cachedMinimapStartX != startTileX ||
		m_cachedMinimapEndX != endTileX ||
		m_cachedMinimapStartY != startTileY ||
		m_cachedMinimapEndY != endTileY ||
		m_cachedMinimapSampleStep != sampleStep ||
		m_cachedMinimapExpanded != expanded;

	if (rebuildMinimap)
	{
		m_minimapRuns.clear();
		m_minimapRuns.reserve(sampleWidth * sampleHeight / 2);

		for (int tileY = startTileY; tileY <= endTileY; tileY += sampleStep)
		{
			int runStartSample = -1;
			int runLength = 0;
			unsigned int runColorKey = 0;
			float runColorR = 0.0f;
			float runColorG = 0.0f;
			float runColorB = 0.0f;
			const int sampleY = (tileY - startTileY) / sampleStep;

			auto flushRun = [&]()
			{
				if (runLength <= 0)
					return;

				MinimapRunState run;
				run.x = static_cast<float>(runStartSample) + static_cast<float>(runLength) * 0.5f;
				run.y = -static_cast<float>(sampleY) - 0.5f;
				run.width = static_cast<float>(runLength);
				run.colorR = runColorR;
				run.colorG = runColorG;
				run.colorB = runColorB;
				m_minimapRuns.push_back(run);
				runStartSample = -1;
				runLength = 0;
				runColorKey = 0;
			};

			for (int tileX = startTileX; tileX <= endTileX; tileX += sampleStep)
			{
				const int sampleX = (tileX - startTileX) / sampleStep;
				const BlockTile& tile = m_world.blocks[tileY * m_blockWidth + tileX];
				if (!IsMapTileRevealed(tileX, tileY) || tile.visible == 0)
				{
					flushRun();
					continue;
				}

				float colorR = 0.0f;
				float colorG = 0.0f;
				float colorB = 0.0f;
				const unsigned int colorKey = getMapColor(tile.tileIndex, colorR, colorG, colorB);
				if (runLength > 0 && colorKey == runColorKey)
				{
					++runLength;
					continue;
				}

				flushRun();
				runStartSample = sampleX;
				runLength = 1;
				runColorKey = colorKey;
				runColorR = colorR;
				runColorG = colorG;
				runColorB = colorB;
			}

			flushRun();
		}

		m_cachedMinimapStartX = startTileX;
		m_cachedMinimapEndX = endTileX;
		m_cachedMinimapStartY = startTileY;
		m_cachedMinimapEndY = endTileY;
		m_cachedMinimapSampleStep = sampleStep;
		m_cachedMinimapExpanded = expanded;
		m_minimapDirty = false;
	}

	for (const MinimapRunState& run : m_minimapRuns)
	{
		DrawSolidRect(
			mapLeft + run.x * pixelSize,
			mapTop + run.y * pixelSize,
			run.width * pixelSize,
			pixelSize,
			run.colorR,
			run.colorG,
			run.colorB,
			0.95f,
			mapTileDepth);
	}

	auto drawMapMarker = [&](float worldX, float worldY, float markerSize, float colorR, float colorG, float colorB, bool requireReveal)
	{
		const int tileX = WorldToTileX(worldX);
		const int tileY = WorldToTileY(worldY);
		if (tileX < startTileX || tileX > endTileX || tileY < startTileY || tileY > endTileY)
			return;
		if (requireReveal && !IsMapTileRevealed(tileX, tileY))
			return;

		const float normalizedX = (static_cast<float>(tileX - startTileX)) / static_cast<float>(sampleStep);
		const float normalizedY = (static_cast<float>(tileY - startTileY)) / static_cast<float>(sampleStep);
		const float markerX = mapLeft + normalizedX * pixelSize;
		const float markerY = mapTop - normalizedY * pixelSize;
		DrawSolidRect(markerX, markerY, markerSize, markerSize, colorR, colorG, colorB, 1.0f, mapMarkerDepth);
	};

	drawMapMarker(m_player.x, m_player.y, expanded ? 6.0f : 5.0f, 0.35f, 0.74f, 1.0f, false);
	const float mapWorldCenterX = m_worldOriginX + (static_cast<float>(startTileX + endTileX) * 0.5f) * m_tileSize;
	const float mapWorldCenterY = m_worldOriginY - (static_cast<float>(startTileY + endTileY) * 0.5f) * m_tileSize;
	const collib::AABB mapWorldArea = collib::MakeAABB(
		mapWorldCenterX,
		mapWorldCenterY,
		static_cast<float>(endTileX - startTileX + 1) * m_tileSize,
		static_cast<float>(endTileY - startTileY + 1) * m_tileSize);
	QueryMonstersInAABB(mapWorldArea, m_monsterQueryScratch);
	for (int monsterIndex : m_monsterQueryScratch)
	{
		const MonsterState& monster = m_monsters[monsterIndex];
		drawMapMarker(monster.x, monster.y, expanded ? 4.2f : 3.5f, 1.0f, 0.22f, 0.18f, true);
	}

	if (expanded)
	{
		RectOutlineDesc outlineDesc;
		outlineDesc.positionX = mapLeft + mapWidth * 0.5f;
		outlineDesc.positionY = mapTop - mapHeight * 0.5f;
		outlineDesc.width = mapWidth + 8.0f;
		outlineDesc.height = mapHeight + 8.0f;
		outlineDesc.thickness = 1.5f;
		outlineDesc.colorR = UiThemeCreamR;
		outlineDesc.colorG = UiThemeCreamG;
		outlineDesc.colorB = UiThemeCreamB;
		outlineDesc.colorA = 0.8f;
		outlineDesc.depth = mapOutlineDepth;
		m_renderer->DrawRectOutline(outlineDesc);
	}
}

void GameEngine::DrawPlayerStatus()
{
	const UiRect panel = GetRightPanelRect(0);
	const float x = panel.left + 12.0f;
	const float y = panel.top - 28.0f;
	const float healthRatio = Clamp01(static_cast<float>(m_playerHealth) / static_cast<float>(GetPlayerMaxHealth()));

	char hpText[24] = {};
	std::snprintf(hpText, sizeof(hpText), "체력 %d/%d", m_playerHealth, GetPlayerMaxHealth());
	DrawUiText(x, y, hpText, 1.35f, UiThemeCreamR, UiThemeCreamG, UiThemeCreamB, 1.0f, -8.0f);
	DrawSolidRect(x + 67.0f, y - 16.0f, 122.0f, 7.0f,
		UiThemeBodyDarkR, UiThemeBodyDarkG, UiThemeBodyDarkB, 0.92f, -7.0f);
	DrawSolidRect(x + 6.0f + 122.0f * healthRatio * 0.5f, y - 16.0f,
		122.0f * healthRatio, 5.0f, 0.25f, 0.92f, 0.48f, 1.0f, -8.0f);
}

void GameEngine::DrawNetworkStatus()
{
	if (m_networkMode == NetworkConfig::Mode::SinglePlayer)
		return;

	char peerText[80] = {};
	BuildNetworkPeerListText(peerText, sizeof(peerText));
	char statusText[128] = {};
	if (m_networkMode == NetworkConfig::Mode::Client && !m_networkConnected)
		std::snprintf(statusText, sizeof(statusText), "net.join pending");
	else
		std::snprintf(statusText, sizeof(statusText), "net.%s local=%s peers=%s", GetNetworkModeText(), GetLocalNickname(), peerText);

	const UiRect panel = GetRightPanelRect(0);
	DrawUiText(panel.left + 12.0f, panel.top - 50.0f, statusText, 1.20f,
		UiThemeMutedR, UiThemeMutedG, UiThemeMutedB, 0.96f, -8.0f);
}

void GameEngine::DrawPlayerStatsPanel()
{
	const UiRect panel = GetLogPanelRect();
	DrawUiPanel(panel, "log");
	const float x = panel.left + UiTerminalPad + 4.0f;
	float y = panel.top - UiTerminalPad - 6.0f;
	const float lineHeight = std::clamp(panel.height * 0.062f, 13.0f, 17.0f);
	const float maxTextWidth = (std::max)(80.0f, panel.width - UiTerminalPad * 2.0f - 8.0f);

	auto drawLogLine = [this, x, maxTextWidth, &y, lineHeight](const char* text)
	{
		y = DrawUiWrappedText(x, y, text, 0.88f, maxTextWidth, lineHeight,
			1.0f, 0.96f, 0.82f, 0.98f, -8.0f);
	};

	if (m_statusTextTimer > 0.0f && m_statusText[0] != '\0')
	{
		char statusLine[128] = {};
		std::snprintf(statusLine, sizeof(statusLine), "%s", m_statusText.data());
		drawLogLine(statusLine);
	}

	drawLogLine("월드가 갱신되었습니다");
	drawLogLine("아이템이 인벤토리에 추가되었습니다");
	drawLogLine("제작 요청이 처리되었습니다");
	drawLogLine("블록 배치 가능 위치를 확인했습니다");
	drawLogLine("주변 몬스터 상태를 동기화했습니다");
	drawLogLine("지도 정보가 갱신되었습니다");
	drawLogLine("장비 슬롯을 검사했습니다");
	drawLogLine("월드 데이터가 저장되었습니다");
	drawLogLine("현재 선택 슬롯을 유지합니다");
}

void GameEngine::DrawMainActionPanel()
{
	const UiRect panel = GetRightPanelRect(2);
	DrawUiPanel(panel, "main-actions");

	const float listLeft = panel.left + UiTerminalListPad;
	const float listTop = panel.top - UiTerminalListTopGap;
	const float listBottom = panel.top - panel.height + UiTerminalListBottomPad;
	const float trackX = panel.left + panel.width - UiTerminalListPad - UiTerminalScrollbarWidth * 0.5f;
	const float listWidth = (std::max)(24.0f, trackX - UiTerminalScrollbarWidth * 0.5f - 4.0f - listLeft);
	const float rowHeight = std::clamp(panel.height * 0.056f, 12.0f, 15.0f);
	const float rowCenterX = listLeft + listWidth * 0.5f;

	float cursorX = 0.0f;
	float cursorY = 0.0f;
	const bool hasCursor = GetCursorViewPosition(cursorX, cursorY);
	const int visibleRows = (std::max)(1, static_cast<int>((listTop - listBottom) / rowHeight));
	const int safeScrollOffset = std::clamp(m_mainActionScrollOffset, 0, (std::max)(0, MainActionLabelCount - visibleRows));
	const int lastAction = (std::min)(MainActionLabelCount, safeScrollOffset + visibleRows);
	const int maxScrollOffset = (std::max)(0, MainActionLabelCount - visibleRows);

	for (int actionIndex = safeScrollOffset; actionIndex < lastAction; ++actionIndex)
	{
		const int visibleIndex = actionIndex - safeScrollOffset;
		const float rowCenterY = listTop - rowHeight * (static_cast<float>(visibleIndex) + 0.5f);
		const bool hovered = hasCursor && IsPointInsideRect(cursorX, cursorY, rowCenterX, rowCenterY, listWidth, rowHeight - 2.0f);
		const bool selected = actionIndex == 5;
		DrawSolidRect(rowCenterX, rowCenterY, listWidth, rowHeight - 2.0f,
			selected || hovered ? UiThemeAmberR : UiThemeBodyR,
			selected || hovered ? UiThemeAmberG : UiThemeBodyG,
			selected || hovered ? UiThemeAmberB : UiThemeBodyB,
			selected || hovered ? 0.96f : 0.72f,
			-6.8f);

		DrawUiText(listLeft + 7.0f, rowCenterY + 4.0f, MainActionLabels[actionIndex], 0.94f,
			selected || hovered ? 1.0f : UiThemeCreamR,
			selected || hovered ? 0.96f : UiThemeCreamG,
			selected || hovered ? 0.82f : UiThemeCreamB,
			1.0f, -8.0f);
	}

	const float trackHeight = (std::max)(8.0f, listTop - listBottom);
	const float trackCenterY = listTop - trackHeight * 0.5f;
	DrawSolidRect(trackX, trackCenterY, 2.0f, trackHeight,
		1.0f, 1.0f, 1.0f, 0.38f, -7.1f);
	const bool canScroll = maxScrollOffset > 0;
	const float thumbHeight = canScroll
		? (std::max)(14.0f, trackHeight * (static_cast<float>(visibleRows) / static_cast<float>(MainActionLabelCount)))
		: trackHeight;
	const float scrollRange = (std::max)(0.0f, trackHeight - thumbHeight);
	const float scrollT = canScroll ? static_cast<float>(safeScrollOffset) / static_cast<float>(maxScrollOffset) : 0.0f;
	const float thumbCenterY = listTop - thumbHeight * 0.5f - scrollRange * scrollT;
	DrawSolidRect(trackX, thumbCenterY, 4.0f, thumbHeight,
		1.0f, 1.0f, 1.0f, canScroll ? 0.90f : 0.44f, -7.8f);
}

void GameEngine::DrawEquipmentTooltip()
{
	float cursorX = 0.0f;
	float cursorY = 0.0f;
	if (!GetCursorViewPosition(cursorX, cursorY))
		return;

	const int slot = GetInventorySlotAt(cursorX, cursorY);
	if (!IsInventoryWeaponSlot(slot))
		return;
	if (m_inventory.GetSlotCount(slot) <= 0)
		return;

	const int item = GetInventorySlotItem(slot);
	const EquipmentStats stats = GetEquipmentStatsForSlot(item);
	if (stats.name[0] == '\0')
		return;

	const float panelWidth = 150.0f;
	const float panelHeight = 96.0f;
	float left = cursorX + 16.0f;
	float top = cursorY + 72.0f;
	const float viewHalfWidth = GetViewHalfWidth();
	const float viewHalfHeight = GetViewHalfHeight();
	if (left + panelWidth > viewHalfWidth - 8.0f)
		left = cursorX - panelWidth - 16.0f;
	if (top > viewHalfHeight - 8.0f)
		top = viewHalfHeight - 8.0f;
	if (top - panelHeight < -viewHalfHeight + 8.0f)
		top = -viewHalfHeight + panelHeight + 8.0f;

	const float centerX = left + panelWidth * 0.5f;
	const float centerY = top - panelHeight * 0.5f;
	constexpr float tooltipIconDepth = -11.8f;
	constexpr float tooltipTextShadowDepth = -11.9f;
	constexpr float tooltipTextDepth = -12.2f;

	DrawSolidRect(centerX + 4.0f, centerY - 4.0f, panelWidth, panelHeight,
		0.0f, 0.0f, 0.0f, 0.42f, -10.9f);
	DrawSolidRect(centerX, centerY, panelWidth, panelHeight,
		UiThemeBodyDarkR, UiThemeBodyDarkG, UiThemeBodyDarkB, 0.96f, -11.1f);
	DrawSolidRect(centerX, top - 18.0f, panelWidth - 14.0f, 1.0f,
		UiThemeCreamR, UiThemeCreamG, UiThemeCreamB, 0.26f, -11.35f);
	DrawSolidRect(left + 14.0f, top - 10.0f, 22.0f, 2.0f,
		UiThemeAmberR, UiThemeAmberG, UiThemeAmberB, 0.92f, -11.4f);
	RectOutlineDesc tooltipOutline;
	tooltipOutline.positionX = centerX;
	tooltipOutline.positionY = centerY;
	tooltipOutline.width = panelWidth;
	tooltipOutline.height = panelHeight;
	tooltipOutline.thickness = 1.0f;
	tooltipOutline.colorR = UiThemeCreamR;
	tooltipOutline.colorG = UiThemeCreamG;
	tooltipOutline.colorB = UiThemeCreamB;
	tooltipOutline.colorA = 0.52f;
	tooltipOutline.depth = -11.45f;
	m_renderer->DrawRectOutline(tooltipOutline);

	auto drawTipText = [this, tooltipTextShadowDepth, tooltipTextDepth](float x, float y, const char* text, float size,
		float colorR, float colorG, float colorB)
	{
		DrawText(x + 1.0f, y - 1.0f, text, size, UiThemeShadowR, UiThemeShadowG, UiThemeShadowB, 0.76f, tooltipTextShadowDepth);
		DrawText(x, y, text, size, colorR, colorG, colorB, 1.0f, tooltipTextDepth);
	};

	char attackText[32] = {};
	char defenseText[32] = {};
	char chopText[32] = {};
	std::snprintf(attackText, sizeof(attackText), "공격 +%d", stats.attackBonus);
	std::snprintf(defenseText, sizeof(defenseText), "방어 +%d", stats.defenseBonus);
	std::snprintf(chopText, sizeof(chopText), "벌목 %.1f배", stats.chopSpeedMultiplier);

	DrawWeaponIcon(left + 21.0f, top - 25.0f, 30.0f, tooltipIconDepth, 1.0f, item);
	drawTipText(left + 43.0f, top - 14.0f, stats.name, 1.9f,
		UiThemeCreamR, UiThemeCreamG, UiThemeCreamB);
	drawTipText(left + 43.0f, top - 32.0f, "사용 가능", 1.35f,
		0.76f, 0.70f, 0.64f);
	drawTipText(left + 12.0f, top - 53.0f, attackText, 1.45f, 0.80f, 0.72f, 0.64f);
	drawTipText(left + 12.0f, top - 68.0f, defenseText, 1.45f, 0.80f, 0.72f, 0.64f);
	drawTipText(left + 12.0f, top - 83.0f, chopText, 1.45f, 0.74f, 0.68f, 0.62f);
	drawTipText(left + 93.0f, top - 83.0f, stats.role, 1.25f, 0.70f, 0.64f, 0.58f);
}

void GameEngine::DrawCraftingPanel()
{
	const UiRect panel = GetRightPanelRect(1);
	DrawUiPanel(panel, "craft-items");
	const CraftingPanelLayout layout = GetCraftingPanelLayout();
	ClampCraftingScrollOffset();

	auto drawPanelText = [this](float x, float y, const char* text, float size,
		float colorR, float colorG, float colorB, float depth)
	{
		DrawUiText(x, y, text, size, colorR, colorG, colorB, 1.0f, depth);
	};

	float cursorX = 0.0f;
	float cursorY = 0.0f;
	const int hoveredRecipe = GetCursorViewPosition(cursorX, cursorY) ? GetCraftingRecipeAt(cursorX, cursorY) : -1;

	struct CraftingRow
	{
		const char* action;
		bool craftable;
	};

	auto getRow = [this](int recipeIndex)
	{
		const bool tableNearby = IsCraftingTableNearby();
		int visibleIndex = 0;
		for (const CraftingRecipeInfo& recipe : CraftingRecipes)
		{
			if (recipe.requiresTable && !tableNearby)
				continue;
			if (visibleIndex != recipeIndex)
			{
				++visibleIndex;
				continue;
			}

			bool craftable = true;
			for (const RecipeIngredient& ingredient : recipe.ingredients)
			{
				if (ingredient.item == InventoryEmptyItem || ingredient.amount <= 0)
					continue;
				if (GetInventoryItemCount(ingredient.item) < ingredient.amount)
				{
					craftable = false;
					break;
				}
			}
			return CraftingRow{ recipe.action, craftable };
		}

		return CraftingRow{ "", false };
	};

	const int recipeCount = GetCraftingRecipeCount();
	const int visibleRows = GetVisibleCraftingRecipeRows();
	const int firstRecipe = m_craftingScrollOffset;
	const int lastRecipe = (std::min)(recipeCount, firstRecipe + visibleRows);
	const bool canScroll = recipeCount > visibleRows;
	const float scrollbarWidth = UiTerminalScrollbarWidth;
	const float listWidth = layout.width - scrollbarWidth;
	for (int recipe = firstRecipe; recipe < lastRecipe; ++recipe)
	{
		const CraftingRow row = getRow(recipe);
		const int visibleIndex = recipe - firstRecipe;
		const float rowCenterX = layout.left + listWidth * 0.5f;
		const float rowCenterY = layout.firstRowCenterY - visibleIndex * layout.rowHeight;
		const bool hovered = hoveredRecipe == recipe;

		DrawSolidRect(rowCenterX, rowCenterY, listWidth, layout.rowHeight - 2.0f,
			hovered ? 0.35f : (row.craftable ? UiThemeBodyR : 0.035f),
			hovered ? 0.20f : (row.craftable ? UiThemeBodyG : 0.060f),
			hovered ? 0.06f : (row.craftable ? UiThemeBodyB : 0.075f),
			hovered ? 0.96f : 0.72f,
			-6.8f);
		DrawSolidRect(rowCenterX, rowCenterY + layout.rowHeight * 0.5f - 1.0f, listWidth, 1.0f,
			hovered ? UiThemeAmberR : UiThemeCreamR,
			hovered ? UiThemeAmberG : UiThemeCreamG,
			hovered ? UiThemeAmberB : UiThemeCreamB,
			hovered ? 0.62f : 0.22f, -7.1f);
		DrawSolidRect(rowCenterX, rowCenterY - layout.rowHeight * 0.5f + 1.0f, listWidth, 1.0f,
			UiThemeShadowR, UiThemeShadowG, UiThemeShadowB, 0.70f, -7.1f);

		char commandText[96] = {};
		std::snprintf(commandText, sizeof(commandText), "%s", row.action);
		drawPanelText(layout.left + 7.0f, rowCenterY + 2.0f, commandText, 0.94f,
			hovered ? UiThemeAmberR : (row.craftable ? UiThemeCreamR : UiThemeMutedR),
			hovered ? UiThemeAmberG : (row.craftable ? UiThemeCreamG : UiThemeMutedG),
			hovered ? UiThemeAmberB : (row.craftable ? UiThemeCreamB : UiThemeMutedB),
			-8.0f);
	}

	const float trackWidth = scrollbarWidth;
	const float trackHeight = layout.height;
	const float trackX = layout.left + layout.width - trackWidth * 0.5f;
	const float trackCenterY = layout.top - layout.height * 0.5f;
	DrawSolidRect(trackX, trackCenterY, trackWidth, trackHeight,
		UiThemeBodyDarkR, UiThemeBodyDarkG, UiThemeBodyDarkB, 0.92f, -7.0f);
	DrawSolidRect(layout.left + listWidth + 0.5f, trackCenterY, 1.0f, trackHeight,
		1.0f, 1.0f, 1.0f, 0.38f, -7.2f);

	const float thumbHeight = canScroll
		? (std::max)(18.0f, trackHeight * (static_cast<float>(visibleRows) / static_cast<float>(recipeCount)))
		: trackHeight;
	const float scrollRange = (std::max)(0.0f, trackHeight - thumbHeight);
	const float scrollT = canScroll
		? static_cast<float>(m_craftingScrollOffset) / static_cast<float>(GetMaxCraftingScrollOffset())
		: 0.0f;
	const float thumbCenterY = layout.top - thumbHeight * 0.5f - scrollRange * scrollT;
	DrawSolidRect(trackX, thumbCenterY, trackWidth - 2.0f, thumbHeight,
		1.0f, 1.0f, 1.0f, canScroll ? 0.90f : 0.44f, -7.8f);
	if (!canScroll)
	{
		DrawSolidRect(trackX, trackCenterY, 2.0f, trackHeight - 4.0f,
			UiThemeBodyDarkR, UiThemeBodyDarkG, UiThemeBodyDarkB, 0.44f, -8.0f);
	}
}

void GameEngine::DrawDebugLogPanel()
{
	const UiRect panel = GetLogPanelRect();
	DrawUiPanel(panel, m_showRenderStats ? "performance" : "log");
	const float x = panel.left + UiTerminalPad + 6.0f;
	const float y = panel.top - UiTerminalHeaderHeight - 25.0f;
	const float maxTextWidth = (std::max)(24.0f, panel.width - UiTerminalPad * 2.0f - 12.0f);
	const char* text = m_statusTextTimer > 0.0f ? m_statusText.data() : nullptr;

	char fpsText[24] = {};
	char frameText[24] = {};
	char capText[24] = {};
	std::snprintf(fpsText, sizeof(fpsText), "fps %03d", static_cast<int>(m_displayFps + 0.5f));
	std::snprintf(frameText, sizeof(frameText), "dt %.1fms", m_displayFrameMs);
	std::snprintf(capText, sizeof(capText), m_frameLimitEnabled ? "cap %03d" : "cap off", m_targetRefreshRate);
	const float frameTextX = panel.left + panel.width * 0.34f;
	const float capTextX = panel.left + panel.width * 0.61f;
	DrawUiText(x, y, fpsText, 1.32f, UiThemeCreamR, UiThemeCreamG, UiThemeCreamB, 1.0f, -8.0f);
	DrawUiText(frameTextX, y, frameText, 1.20f, UiThemeMutedR, UiThemeMutedG, UiThemeMutedB, 1.0f, -8.0f);
	DrawUiText(capTextX, y, capText, 1.16f, 0.56f, 0.66f, 0.72f, 1.0f, -8.0f);

	float cursorX = 0.0f;
	float cursorY = 0.0f;
	const bool hasCursor = GetCursorViewPosition(cursorX, cursorY);
	const float buttonY = panel.top - UiTerminalHeaderHeight - 76.0f;
	const float buttonHeight = (std::max)(24.0f, panel.height * 0.10f);
	const float perfX = panel.left + panel.width * 0.16f;
	const float capX = panel.left + panel.width * 0.36f;
	const float revealX = panel.left + panel.width * 0.62f;

	auto drawDebugButton = [&](float centerX, float width, const char* text)
	{
		const bool hovered = hasCursor && IsPointInsideRect(cursorX, cursorY, centerX, buttonY, width, buttonHeight);
		DrawSolidRect(centerX, buttonY, width, buttonHeight,
			hovered ? 0.30f : UiThemeBodyR,
			hovered ? 0.40f : UiThemeBodyG,
			hovered ? 0.48f : UiThemeBodyB,
			0.94f, -8.5f);

		RectOutlineDesc outline;
		outline.positionX = centerX;
		outline.positionY = buttonY;
		outline.width = width;
		outline.height = buttonHeight;
		outline.thickness = hovered ? 1.8f : 1.1f;
		outline.colorR = hovered ? UiThemeCreamR : UiThemeMutedR;
		outline.colorG = hovered ? UiThemeCreamG : UiThemeMutedG;
		outline.colorB = hovered ? UiThemeCreamB : UiThemeMutedB;
		outline.colorA = hovered ? 0.96f : 0.82f;
		outline.depth = -8.9f;
		m_renderer->DrawRectOutline(outline);

		DrawCenteredUiText(centerX, buttonY + 4.5f, text, 1.02f,
			hovered ? UiThemeCreamR : UiThemeMutedR,
			hovered ? UiThemeCreamG : UiThemeMutedG,
			hovered ? UiThemeCreamB : UiThemeMutedB,
			1.0f, -9.1f);
	};

	char perfText[24] = {};
	std::snprintf(perfText, sizeof(perfText), m_showRenderStats ? "trace on" : "trace");
	drawDebugButton(perfX, panel.width * 0.16f, perfText);
	drawDebugButton(capX, panel.width * 0.16f, "limit");
	drawDebugButton(revealX, panel.width * 0.22f, "map.reveal");

	float lineY = buttonY - buttonHeight * 0.85f;
	const float minLineY = panel.top - panel.height + 12.0f;
	auto drawLine = [this, x, maxTextWidth, &lineY, minLineY](const char* lineText, float colorR, float colorG, float colorB)
	{
		if (lineY < minLineY)
			return;

		lineY = DrawUiWrappedText(x, lineY, lineText, 1.18f, maxTextWidth, 16.0f, colorR, colorG, colorB, 1.0f, -8.0f);
	};

	if (m_showRenderStats && m_renderer != nullptr)
	{
		RenderFrameStats stats;
		m_renderer->GetLastFrameStats(stats);

		const float leftX = x;
		const float rightX = panel.left + panel.width * 0.52f;
		auto drawMetric = [this, leftX, rightX, &lineY, minLineY](const char* leftText, const char* rightText)
		{
			if (lineY < minLineY)
				return;

			DrawUiText(leftX, lineY, leftText, 0.84f, 0.50f, 0.62f, 0.66f, 1.0f, -8.0f);
			if (rightText != nullptr && rightText[0] != '\0')
				DrawUiText(rightX, lineY, rightText, 0.84f, 0.50f, 0.62f, 0.66f, 1.0f, -8.0f);
			lineY -= 11.5f;
		};

		char leftText[80] = {};
		char rightText[80] = {};
		std::snprintf(leftText, sizeof(leftText), "프레임시간 %.2fms", m_displayCpuStats.totalMs);
		std::snprintf(rightText, sizeof(rightText), "렌더시간 %.2fms", m_displayCpuStats.renderMs);
		drawMetric(leftText, rightText);

		std::snprintf(leftText, sizeof(leftText), "그리기시간 %.2fms", m_displayCpuStats.drawWorldMs);
		std::snprintf(rightText, sizeof(rightText), "게임갱신 %.2fms", m_displayCpuStats.simulationMs);
		drawMetric(leftText, rightText);

		std::snprintf(leftText, sizeof(leftText), "입력시간 %.2fms", m_displayCpuStats.inputMs);
		std::snprintf(rightText, sizeof(rightText), "몬스터시간 %.2fms", m_displayCpuStats.monstersMs);
		drawMetric(leftText, rightText);

		std::snprintf(leftText, sizeof(leftText), "아이템/통신시간 %.2fms", m_displayCpuStats.itemsMs);
		std::snprintf(rightText, sizeof(rightText), "시간단위 ms");
		drawMetric(leftText, rightText);

		std::snprintf(leftText, sizeof(leftText), "드로우콜 %u 텍스처변경 %u", stats.drawCalls, stats.textureBinds);
		std::snprintf(rightText, sizeof(rightText), "뷰포트변경 %u", stats.viewportChanges);
		drawMetric(leftText, rightText);

		std::snprintf(leftText, sizeof(leftText), "백버퍼 %ux%u", stats.backBufferWidth, stats.backBufferHeight);
		std::snprintf(rightText, sizeof(rightText), "스프라이트배치 %u", stats.spriteBatches);
		drawMetric(leftText, rightText);

		std::snprintf(leftText, sizeof(leftText), "폰트배치 %u 글리프배치 %u", stats.fontBatches, stats.glyphBatches);
		std::snprintf(rightText, sizeof(rightText), "최대배치 %u/%u/%u", stats.maxSpriteBatchQuads, stats.maxFontBatchQuads, stats.maxGlyphBatchQuads);
		drawMetric(leftText, rightText);

		std::snprintf(leftText, sizeof(leftText), "스프라이트합계 %u", stats.spriteQuads);
		std::snprintf(rightText, sizeof(rightText), "UI사각 %u 텍스처 %u", stats.whiteQuads, stats.texturedQuads);
		drawMetric(leftText, rightText);

		std::snprintf(leftText, sizeof(leftText), "폰트문자 %u 글리프스프라이트 %u", stats.fontQuads, stats.glyphQuads);
		std::snprintf(rightText, sizeof(rightText), "텍스트호출 %u", stats.textDrawCalls);
		drawMetric(leftText, rightText);

		std::snprintf(leftText, sizeof(leftText), "타일인스턴스 %u", stats.gridInstances);
		std::snprintf(rightText, sizeof(rightText), "보이는타일 %u", stats.gridVisibleTiles);
		drawMetric(leftText, rightText);

		std::snprintf(leftText, sizeof(leftText), "청크그리기 %u 재생성 %u", stats.gridChunksDrawn, stats.gridChunksRebuilt);
		std::snprintf(rightText, sizeof(rightText), "청크캐시무효 %u", stats.gridCacheInvalidations);
		drawMetric(leftText, rightText);

		std::snprintf(leftText, sizeof(leftText), "동적버퍼 업로드 %uKB", stats.dynamicBufferUploadBytes / 1024u);
		std::snprintf(rightText, sizeof(rightText), "타일업로드 %uKB", stats.gridUploadBytes / 1024u);
		drawMetric(leftText, rightText);

		std::snprintf(leftText, sizeof(leftText), "스프라이트업로드 %uKB", stats.spriteUploadBytes / 1024u);
		std::snprintf(rightText, sizeof(rightText), "폰트업로드 %uKB", stats.fontUploadBytes / 1024u);
		drawMetric(leftText, rightText);

		std::snprintf(leftText, sizeof(leftText), "텍스트글자 %u 폰트캐시 %u", stats.textGlyphs, stats.fontGlyphsCached);
		std::snprintf(rightText, sizeof(rightText), "폰트아틀라스업로드 %u", stats.fontAtlasUploads);
		drawMetric(leftText, rightText);

		const char* suspect = "균형";
		if (stats.gridChunksRebuilt > 0 || stats.gridCacheInvalidations > 0)
			suspect = "타일 캐시 재생성";
		else if (stats.textureBinds > 24 || stats.drawCalls > 28)
			suspect = "배치/텍스처 전환";
		else if (stats.fontQuads > stats.spriteQuads && stats.textGlyphs > 120)
			suspect = "텍스트/UI";
		else if (stats.dynamicBufferUploadBytes > 256u * 1024u)
			suspect = "버퍼 업로드";
		else if (m_displayCpuStats.monstersMs > m_displayCpuStats.renderMs && m_displayCpuStats.monstersMs > 0.35f)
			suspect = "몬스터/충돌";

		std::snprintf(leftText, sizeof(leftText), "의심 병목 %s", suspect);
		std::snprintf(rightText, sizeof(rightText), "아틀라스 %uKB", stats.fontAtlasUploadBytes / 1024u);
		drawMetric(leftText, rightText);
	}

	if (text != nullptr && text[0] != '\0')
	{
		drawLine(text, UiThemeCreamR, UiThemeCreamG, UiThemeCreamB);
	}
	else
	{
		if (m_networkMode != NetworkConfig::Mode::SinglePlayer)
		{
			char peerText[80] = {};
			BuildNetworkPeerListText(peerText, sizeof(peerText));
			char networkText[128] = {};
			if (m_networkMode == NetworkConfig::Mode::Client && !m_networkConnected)
				std::snprintf(networkText, sizeof(networkText), "net.join pending");
			else
				std::snprintf(networkText, sizeof(networkText), "net.%s local=%s peers=%s", GetNetworkModeText(), GetLocalNickname(), peerText);
			drawLine(networkText, UiThemeMutedR, UiThemeMutedG, UiThemeMutedB);
		}
	}

	char selectedText[80] = {};
	const int selectedSlot = m_inventory.GetSelectedSlot();
	const int selectedItem = GetSelectedInventoryItem();
	const char* selectedName = selectedItem != InventoryEmptyItem ? GetInventoryItemName(selectedItem) : "빈칸";
	std::snprintf(selectedText, sizeof(selectedText), "선택 슬롯 %02d  %s  수량 %d",
		selectedSlot + 1,
		selectedName,
		m_inventory.GetSlotCount(selectedSlot));
	drawLine(selectedText, UiThemeMutedR, UiThemeMutedG, UiThemeMutedB);
}

void GameEngine::DrawUiShell()
{
	const UiRect game = GetGameViewportRect();
	if (game.width <= 0.0f || game.height <= 0.0f)
		return;

	const float healthX = game.left + 42.0f;
	const float healthY = game.top - 20.0f;
	const float healthRatio = Clamp01(static_cast<float>(m_playerHealth) / static_cast<float>(GetPlayerMaxHealth()));
	DrawSolidRect(healthX + 28.0f, healthY + 2.0f, 58.0f, 4.0f,
		0.08f, 0.12f, 0.06f, 0.92f, -8.1f);
	DrawSolidRect(healthX + healthRatio * 29.0f, healthY + 2.0f, healthRatio * 58.0f, 3.0f,
		0.35f, 0.95f, 0.22f, 1.0f, -8.3f);
	DrawUiText(healthX, healthY - 10.0f, "플레이어 체력", 0.96f,
		1.0f, 0.96f, 0.82f, 0.96f, -8.4f);

}

void GameEngine::DrawUiPanel(const UiRect& rect, const char* title, float depth)
{
	if (rect.width <= 0.0f || rect.height <= 0.0f)
		return;

	const float centerX = rect.left + rect.width * 0.5f;
	const float centerY = rect.top - rect.height * 0.5f;
	const float left = rect.left;
	const float top = rect.top;
	const float width = rect.width;
	const float height = rect.height;

	DrawSolidRect(centerX, centerY, width, height,
		0.0f, 0.0f, 0.0f, 0.96f, depth + 0.10f);

	RectOutlineDesc frameOutline;
	frameOutline.positionX = centerX;
	frameOutline.positionY = centerY;
	frameOutline.width = width - 1.0f;
	frameOutline.height = height - 1.0f;
	frameOutline.thickness = 1.0f;
	frameOutline.colorR = 1.0f;
	frameOutline.colorG = 1.0f;
	frameOutline.colorB = 1.0f;
	frameOutline.colorA = 0.90f;
	frameOutline.depth = depth - 0.55f;
	m_renderer->DrawRectOutline(frameOutline);

	const char* displayTitle = GetUiPanelDisplayTitle(title);
	if (displayTitle != nullptr && displayTitle[0] != '\0')
	{
		DrawText(left + 8.0f, top - 7.0f, displayTitle, 0.78f,
			1.0f, 1.0f, 1.0f, 1.0f, depth - 1.35f);
	}
}

void GameEngine::DrawPerformanceOverlay()
{
	if (!m_showRenderStats || m_renderer == nullptr)
		return;

	RenderFrameStats stats;
	m_renderer->GetLastFrameStats(stats);

	const float x = -GetViewHalfWidth() + UiScreenSafeInset + 10.0f;
	float y = GetViewHalfHeight() - 14.0f;
	const float lineHeight = 12.0f;
	const float size = 1.10f;
	const auto drawOverlayLine = [this, x, &y, lineHeight, size](const char* text)
	{
		DrawText(x + 1.0f, y - 1.0f, text, size, 0.0f, 0.0f, 0.0f, 0.78f, -11.8f);
		DrawText(x, y, text, size, 1.0f, 1.0f, 1.0f, 1.0f, -12.0f);
		y -= lineHeight;
	};

	char text[96] = {};
	std::snprintf(text, sizeof(text), "FPS %.0f  %.1fms  %s",
		m_displayFps, m_displayFrameMs, m_frameLimitEnabled ? "CAP ON" : "CAP OFF");
	drawOverlayLine(text);
	std::snprintf(text, sizeof(text), "CPU %.2fms  SIM %.2fms  RENDER %.2fms",
		m_displayCpuStats.totalMs, m_displayCpuStats.simulationMs, m_displayCpuStats.renderMs);
	drawOverlayLine(text);
	std::snprintf(text, sizeof(text), "DRAW %.2fms  CALLS %u  TEX %u",
		m_displayCpuStats.drawWorldMs, stats.drawCalls, stats.textureBinds);
	drawOverlayLine(text);
	std::snprintf(text, sizeof(text), "TILES %u  CHUNKS %u/%u",
		stats.gridVisibleTiles, stats.gridChunksDrawn, stats.gridChunksRebuilt);
	drawOverlayLine(text);
	std::snprintf(text, sizeof(text), "UPLOAD DYN %uKB  GRID %uKB",
		stats.dynamicBufferUploadBytes / 1024u, stats.gridUploadBytes / 1024u);
	drawOverlayLine(text);
}

void GameEngine::DrawClassicUiPanel(const UiRect& rect, const char* title, float depth)
{
	DrawUiPanel(rect, title, depth);
}

void GameEngine::DrawUiText(float x, float y, const char* text, float pixelSize, float colorR, float colorG, float colorB, float colorA, float depth)
{
	DrawText(x, y, text, pixelSize, colorR, colorG, colorB, colorA, depth);
}

void GameEngine::DrawCenteredUiText(float centerX, float y, const char* text, float pixelSize, float colorR, float colorG, float colorB, float colorA, float depth)
{
	DrawUiText(centerX - GetUiTextWidth(text, pixelSize) * 0.5f, y, text, pixelSize, colorR, colorG, colorB, colorA, depth);
}

float GameEngine::GetUiTextWidth(const char* text, float pixelSize) const
{
	if (text == nullptr || text[0] == '\0' || pixelSize <= 0.0f)
		return 0.0f;

	float currentLineWidth = 0.0f;
	float maxLineWidth = 0.0f;
	const unsigned char* cursor = reinterpret_cast<const unsigned char*>(text);
	while (*cursor != '\0')
	{
		const unsigned char lead = *cursor;
		if (lead == '\r')
		{
			++cursor;
			continue;
		}
		if (lead == '\n')
		{
			maxLineWidth = (std::max)(maxLineWidth, currentLineWidth);
			currentLineWidth = 0.0f;
			++cursor;
			continue;
		}
		if (lead == '\t')
		{
			currentLineWidth += pixelSize * 16.0f;
			++cursor;
			continue;
		}
		if (lead == ' ')
		{
			currentLineWidth += pixelSize * 4.0f;
			++cursor;
			continue;
		}
		if (lead < 0x80)
		{
			currentLineWidth += pixelSize * 6.0f;
			++cursor;
			continue;
		}

		size_t byteCount = 1;
		if ((lead & 0xE0) == 0xC0)
			byteCount = 2;
		else if ((lead & 0xF0) == 0xE0)
			byteCount = 3;
		else if ((lead & 0xF8) == 0xF0)
			byteCount = 4;

		currentLineWidth += pixelSize * 12.0f;
		for (size_t i = 0; i < byteCount && *cursor != '\0'; ++i)
			++cursor;
	}

	return (std::max)(maxLineWidth, currentLineWidth);
}

float GameEngine::DrawUiWrappedText(float x, float y, const char* text, float pixelSize, float maxWidth, float lineHeight, float colorR, float colorG, float colorB, float colorA, float depth)
{
	if (text == nullptr || text[0] == '\0')
		return y;

	float lineY = y;
	std::string line;

	auto getUtf8SequenceLength = [](const char* cursor)
	{
		if (cursor == nullptr || cursor[0] == '\0')
			return static_cast<size_t>(0);

		const unsigned char lead = static_cast<unsigned char>(cursor[0]);
		size_t byteCount = 1;
		if ((lead & 0xE0) == 0xC0)
			byteCount = 2;
		else if ((lead & 0xF0) == 0xE0)
			byteCount = 3;
		else if ((lead & 0xF8) == 0xF0)
			byteCount = 4;

		for (size_t i = 1; i < byteCount; ++i)
		{
			const unsigned char next = static_cast<unsigned char>(cursor[i]);
			if (next == '\0' || (next & 0xC0) != 0x80)
				return static_cast<size_t>(1);
		}

		return byteCount;
	};

	auto flushLine = [&]()
	{
		if (line.empty())
			return;

		DrawUiText(x, lineY, line.c_str(), pixelSize, colorR, colorG, colorB, colorA, depth);
		lineY -= lineHeight;
		line.clear();
	};

	auto appendWord = [&](std::string word)
	{
		while (GetUiTextWidth(word.c_str(), pixelSize) > maxWidth)
		{
			flushLine();
			std::string segment;
			size_t offset = 0;
			while (offset < word.size())
			{
				const size_t charLength = getUtf8SequenceLength(word.c_str() + offset);
				if (charLength == 0)
					break;

				const std::string nextChar = word.substr(offset, charLength);
				if (!segment.empty() && GetUiTextWidth((segment + nextChar).c_str(), pixelSize) > maxWidth)
					break;

				segment += nextChar;
				offset += charLength;
			}

			if (segment.empty())
				break;

			DrawUiText(x, lineY, segment.c_str(), pixelSize, colorR, colorG, colorB, colorA, depth);
			lineY -= lineHeight;
			word.erase(0, segment.size());
		}

		if (word.empty())
			return;

		if (line.empty())
		{
			line = word;
			return;
		}

		std::string combined = line;
		combined += ' ';
		combined += word;
		if (GetUiTextWidth(combined.c_str(), pixelSize) <= maxWidth)
		{
			line = combined;
			return;
		}

		flushLine();
		line = word;
	};

	const char* cursor = text;
	while (*cursor != '\0')
	{
		if (*cursor == '\n')
		{
			flushLine();
			++cursor;
			continue;
		}

		while (*cursor == ' ' || *cursor == '\t')
			++cursor;

		if (*cursor == '\0')
			break;
		if (*cursor == '\n')
		{
			flushLine();
			++cursor;
			continue;
		}

		const char* wordStart = cursor;
		while (*cursor != '\0' && *cursor != '\n' && *cursor != ' ' && *cursor != '\t')
			++cursor;
		appendWord(std::string(wordStart, static_cast<size_t>(cursor - wordStart)));
	}

	flushLine();
	return lineY;
}

void GameEngine::DrawSolidRect(float centerX, float centerY, float width, float height, float colorR, float colorG, float colorB, float colorA, float depth)
{
	if (width <= 0.0f || height <= 0.0f)
		return;

	SpriteDesc rectDesc;
	rectDesc.textureFile = nullptr;
	rectDesc.positionX = centerX;
	rectDesc.positionY = centerY;
	rectDesc.width = width;
	rectDesc.height = height;
	rectDesc.colorR = colorR;
	rectDesc.colorG = colorG;
	rectDesc.colorB = colorB;
	rectDesc.colorA = colorA;
	rectDesc.depth = depth;
	m_renderer->DrawSprite(rectDesc);
}

void GameEngine::DrawText(float x, float y, const char* text, float pixelSize, float colorR, float colorG, float colorB, float colorA, float depth)
{
	if (text == nullptr || pixelSize <= 0.0f)
		return;
	if (m_renderer == nullptr)
		return;

	TextDesc desc;
	desc.text = text;
	desc.x = x;
	desc.y = y;
	desc.pixelSize = pixelSize;
	desc.colorR = colorR;
	desc.colorG = colorG;
	desc.colorB = colorB;
	desc.colorA = colorA;
	desc.depth = depth;
	m_renderer->DrawText(desc);
}

void GameEngine::DrawGlyph(float x, float y, char glyph, float pixelSize, float colorR, float colorG, float colorB, float colorA, float depth)
{
	const unsigned char* rows = GetGlyphRows(glyph);
	if (rows == nullptr)
		return;

	GlyphSpriteDesc glyphDesc;
	glyphDesc.positionX = x + pixelSize * 2.5f;
	glyphDesc.positionY = y - pixelSize * 3.5f;
	glyphDesc.width = pixelSize * 5.0f;
	glyphDesc.height = pixelSize * 7.0f;
	for (int row = 0; row < 7; ++row)
		glyphDesc.rows[row] = rows[row];

	glyphDesc.colorR = colorR;
	glyphDesc.colorG = colorG;
	glyphDesc.colorB = colorB;
	glyphDesc.colorA = colorA;
	glyphDesc.depth = depth;
	m_renderer->DrawGlyphSprite(glyphDesc);
}

GameEngine::CraftingPanelLayout GameEngine::GetCraftingPanelLayout() const
{
	CraftingPanelLayout layout;
	const UiRect panel = GetRightPanelRect(1);
	layout.left = panel.left + UiTerminalListPad;
	layout.top = panel.top - UiTerminalListTopGap;
	layout.width = (std::max)(24.0f, panel.width - UiTerminalListPad * 2.0f);
	layout.height = (std::max)(24.0f, layout.top - (panel.top - panel.height + UiTerminalListBottomPad));
	layout.rowHeight = std::clamp(panel.height * 0.056f, 12.0f, 15.0f);
	layout.firstRowCenterY = layout.top - layout.rowHeight * 0.5f;
	return layout;
}

GameEngine::UiRect GameEngine::GetGameViewportRect() const
{
	const float viewHalfWidth = GetViewHalfWidth();
	const float viewHalfHeight = GetViewHalfHeight();
	const float fullWidth = viewHalfWidth * 2.0f;
	const float fullHeight = viewHalfHeight * 2.0f;
	const float rightPanelWidth = std::clamp(fullWidth * 0.275f, 260.0f, 520.0f);
	const float gameWidth = fullWidth - rightPanelWidth - 4.0f;
	const float gameHeight = fullHeight * 0.530f;

	UiRect rect;
	rect.left = -viewHalfWidth;
	rect.top = viewHalfHeight;
	rect.width = (std::max)(120.0f, gameWidth);
	rect.height = (std::max)(120.0f, gameHeight);
	return rect;
}

GameEngine::UiRect GameEngine::GetInventoryPanelRect() const
{
	const UiRect game = GetGameViewportRect();
	const float viewHalfHeight = GetViewHalfHeight();
	const float bottomTop = game.top - game.height;
	const float bottomBottom = -viewHalfHeight + 42.0f;
	const float bottomHeight = (std::max)(80.0f, bottomTop - bottomBottom);
	const float targetInventoryWidth = (GetViewHalfWidth() * 2.0f) * 0.375f;

	UiRect rect;
	rect.left = game.left + UiScreenSafeInset;
	rect.top = bottomTop;
	rect.width = (std::max)(220.0f, targetInventoryWidth);
	rect.height = bottomHeight;
	return rect;
}

GameEngine::UiRect GameEngine::GetLogPanelRect() const
{
	const UiRect game = GetGameViewportRect();
	const UiRect inventory = GetInventoryPanelRect();
	const float viewHalfHeight = GetViewHalfHeight();
	const float bottomTop = game.top - game.height - 64.0f;
	const float bottomBottom = -viewHalfHeight + 42.0f;
	const float fullWidth = GetViewHalfWidth() * 2.0f;

	UiRect rect;
	rect.left = inventory.left + inventory.width + 14.0f;
	rect.top = bottomTop;
	rect.width = (std::max)(220.0f, fullWidth * 0.525f);
	if (rect.left + rect.width > GetViewHalfWidth() - 88.0f)
		rect.width = (std::max)(220.0f, GetViewHalfWidth() - 88.0f - rect.left);
	rect.height = (std::max)(80.0f, bottomTop - bottomBottom);
	return rect;
}

GameEngine::UiRect GameEngine::GetRightPanelRect(int panelIndex) const
{
	const UiRect game = GetGameViewportRect();
	const float viewHalfWidth = GetViewHalfWidth();
	const float viewHalfHeight = GetViewHalfHeight();
	const float left = game.left + game.width + 4.0f;
	const float totalHeight = viewHalfHeight * 2.0f;
	const int safeIndex = std::clamp(panelIndex, 0, 2);
	const float topMargin = 14.0f;
	const float panelGap = 18.0f;
	const float mapHeight = totalHeight * 0.220f;
	const float craftHeight = totalHeight * 0.226f;
	const float heroHeight = totalHeight * 0.220f;

	UiRect rect;
	rect.left = left;
	rect.width = (std::max)(120.0f, viewHalfWidth - left - 6.0f);
	if (safeIndex == 0)
	{
		rect.top = viewHalfHeight - topMargin;
		rect.height = mapHeight;
	}
	else if (safeIndex == 1)
	{
		rect.top = viewHalfHeight - topMargin - mapHeight - panelGap;
		rect.height = craftHeight;
	}
	else
	{
		rect.top = viewHalfHeight - topMargin - mapHeight - panelGap - craftHeight - panelGap;
		rect.height = heroHeight;
	}
	return rect;
}

bool GameEngine::GetCursorViewPosition(float& viewX, float& viewY) const
{
	if (m_window == nullptr)
		return false;

	HWND* hwnd = m_window->GetHWND();
	if (hwnd == nullptr || *hwnd == nullptr)
		return false;

	POINT cursorPosition = {};
	if (!GetCursorPos(&cursorPosition) || !ScreenToClient(*hwnd, &cursorPosition))
		return false;

	const float windowWidth = m_window->GetWidth();
	const float windowHeight = m_window->GetHeight();
	if (windowWidth <= 0.0f || windowHeight <= 0.0f)
		return false;

	if (cursorPosition.x < 0 || cursorPosition.y < 0 ||
		cursorPosition.x >= windowWidth || cursorPosition.y >= windowHeight)
	{
		return false;
	}

	const float normalizedX = (static_cast<float>(cursorPosition.x) / windowWidth) * 2.0f - 1.0f;
	const float normalizedY = 1.0f - (static_cast<float>(cursorPosition.y) / windowHeight) * 2.0f;
	viewX = normalizedX * GetViewHalfWidth();
	viewY = normalizedY * GetViewHalfHeight();
	return true;
}

bool GameEngine::GetCursorGameViewPosition(float& viewX, float& viewY) const
{
	float cursorX = 0.0f;
	float cursorY = 0.0f;
	if (!GetCursorViewPosition(cursorX, cursorY))
		return false;

	const UiRect game = GetGameViewportRect();
	if (cursorX < game.left || cursorX > game.left + game.width ||
		cursorY > game.top || cursorY < game.top - game.height)
	{
		return false;
	}

	const float normalizedX = (cursorX - game.left) / game.width;
	const float normalizedY = (game.top - cursorY) / game.height;
	viewX = (normalizedX * 2.0f - 1.0f) * GetViewHalfWidth();
	viewY = (1.0f - normalizedY * 2.0f) * GetViewHalfHeight();
	return true;
}

int GameEngine::GetCraftingRecipeAt(float viewX, float viewY) const
{
	const CraftingPanelLayout layout = GetCraftingPanelLayout();
	if (viewX < layout.left || viewX > layout.left + layout.width ||
		viewY > layout.top || viewY < layout.top - layout.height)
	{
		return -1;
	}
	if (viewX >= layout.left + layout.width - UiTerminalScrollbarWidth)
		return -1;

	const int visibleRows = GetVisibleCraftingRecipeRows();
	const int recipeCount = GetCraftingRecipeCount();
	const int safeScrollOffset = std::clamp(m_craftingScrollOffset, 0, (std::max)(0, recipeCount - visibleRows));
	const int lastRecipe = (std::min)(recipeCount, safeScrollOffset + visibleRows);
	for (int recipe = safeScrollOffset; recipe < lastRecipe; ++recipe)
	{
		const int visibleIndex = recipe - safeScrollOffset;
		const float rowCenterY = layout.firstRowCenterY - visibleIndex * layout.rowHeight;
		if (viewY <= rowCenterY + layout.rowHeight * 0.5f &&
			viewY >= rowCenterY - layout.rowHeight * 0.5f)
		{
			return recipe;
		}
	}

	return -1;
}

int GameEngine::GetInventorySlotAt(float viewX, float viewY) const
{
	const UiRect panel = GetInventoryPanelRect();
	const float gridLeft = panel.left + UiTerminalListPad;
	const float gridRight = panel.left + panel.width - UiTerminalListPad;
	const float gridTop = panel.top - UiTerminalListTopGap;
	const float gridBottom = panel.top - panel.height + UiTerminalListBottomPad + 22.0f;
	const float gridWidth = (std::max)(24.0f, gridRight - gridLeft);
	const float gridHeight = (std::max)(24.0f, gridTop - gridBottom);
	if (gridWidth <= 0.0f || gridHeight <= 0.0f)
		return -1;

	if (viewX < gridLeft || viewX >= gridRight || viewY > gridTop || viewY <= gridBottom)
	{
		return -1;
	}

	const float cellWidth = gridWidth / static_cast<float>(InventoryVisibleColumns);
	const float cellHeight = gridHeight / static_cast<float>(InventoryVisibleRows);
	const int column = static_cast<int>((viewX - gridLeft) / cellWidth);
	const int row = static_cast<int>((gridTop - viewY) / cellHeight);
	if (column < 0 || column >= InventoryVisibleColumns || row < 0 || row >= InventoryVisibleRows)
		return -1;

	const int pageCount = InventoryPageCount;
	const int currentPage = std::clamp(m_inventory.GetSelectedSlot() / InventoryVisibleSlotCount, 0, (std::max)(0, pageCount - 1));
	const int slot = currentPage * InventoryVisibleSlotCount + row * InventoryVisibleColumns + column;
	return slot < InventorySlotCount ? slot : -1;
}

bool GameEngine::IsCursorOverCraftingPanel() const
{
	float cursorX = 0.0f;
	float cursorY = 0.0f;
	if (!GetCursorViewPosition(cursorX, cursorY))
		return false;

	const CraftingPanelLayout layout = GetCraftingPanelLayout();
	return cursorX >= layout.left && cursorX <= layout.left + layout.width &&
		cursorY <= layout.top && cursorY >= layout.top - layout.height;
}

void GameEngine::DrawHoveredBlockOutline()
{
	int tileX = 0;
	int tileY = 0;
	if (!GetHoveredTile(tileX, tileY))
		return;

	const int blockIndex = tileY * m_blockWidth + tileX;
	const bool occupied = m_world.blocks[blockIndex].visible != 0;
	const bool canPlace = CanPlaceSelectedBlockAt(tileX, tileY);

	if (occupied && !IsTileInBlockInteractionRange(tileX, tileY))
		return;

	if (!occupied && !canPlace)
		return;

	RectOutlineDesc outlineDesc;
	outlineDesc.positionX = m_worldOriginX + tileX * m_tileSize - m_cameraX;
	outlineDesc.positionY = m_worldOriginY - tileY * m_tileSize - m_cameraY;
	outlineDesc.width = m_tileSize;
	outlineDesc.height = m_tileSize;
	outlineDesc.thickness = 2.0f;
	outlineDesc.depth = -0.5f;
	if (canPlace)
	{
		outlineDesc.colorR = 0.35f;
		outlineDesc.colorG = 1.0f;
		outlineDesc.colorB = 0.45f;
	}

	m_renderer->DrawRectOutline(outlineDesc);
}


