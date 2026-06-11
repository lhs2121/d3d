#include "pch.h"
#include "GameEngine.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <mmsystem.h>
#include <random>

#pragma comment(lib, "winmm.lib")

namespace
{
	constexpr int KeyA = 0x41;
	constexpr int KeyC = 0x43;
	constexpr int KeyD = 0x44;
	constexpr int KeyS = 0x53;
	constexpr int KeyJump = VK_SPACE;
	constexpr int KeyDebugStats = VK_F1;
	constexpr int KeyFrameLimit = VK_F2;
	constexpr int KeyRevealMap = VK_F3;
	constexpr int KeyMinimap = VK_TAB;
	constexpr float BlockBreakDuration = 0.85f;
	constexpr float InteractionRangeTiles = 6.0f;
	constexpr const WCHAR* BlockAtlasTexture = L"assets\\texture\\block_atlas_extended.png";
	constexpr int BlockAtlasColumns = 16;
	constexpr int BlockAtlasRows = 1;
	struct BackgroundImageLayer
	{
		const WCHAR* textureFile = nullptr;
		float alpha = 1.0f;
		float parallaxX = 0.0f;
		float parallaxY = 0.0f;
		float depth = 5.0f;
	};
	constexpr int BackgroundBiomeCount = 3;
	constexpr int SurfaceBackgroundLayerCount = 4;
	constexpr int CaveBackgroundLayerCount = 4;
	constexpr BackgroundImageLayer SurfaceBackgroundLayers[BackgroundBiomeCount][SurfaceBackgroundLayerCount] =
	{
		{
			{ L"assets\\back\\bg_grass_sky.png", 1.0f, 0.010f, 0.004f, 5.0f },
			{ L"assets\\back\\bg_grass_far.png", 1.0f, 0.040f, 0.012f, 5.0f },
			{ L"assets\\back\\bg_grass_mid.png", 1.0f, 0.085f, 0.024f, 5.0f },
			{ L"assets\\back\\bg_grass_front.png", 1.0f, 0.155f, 0.040f, 5.0f },
		},
		{
			{ L"assets\\back\\bg_desert_sky.png", 1.0f, 0.010f, 0.004f, 5.0f },
			{ L"assets\\back\\bg_desert_far.png", 1.0f, 0.038f, 0.012f, 5.0f },
			{ L"assets\\back\\bg_desert_mid.png", 1.0f, 0.082f, 0.024f, 5.0f },
			{ L"assets\\back\\bg_desert_front.png", 1.0f, 0.160f, 0.042f, 5.0f },
		},
		{
			{ L"assets\\back\\bg_ice_sky.png", 1.0f, 0.010f, 0.004f, 5.0f },
			{ L"assets\\back\\bg_ice_far.png", 1.0f, 0.036f, 0.012f, 5.0f },
			{ L"assets\\back\\bg_ice_mid.png", 1.0f, 0.078f, 0.023f, 5.0f },
			{ L"assets\\back\\bg_ice_front.png", 1.0f, 0.150f, 0.040f, 5.0f },
		},
	};
	constexpr BackgroundImageLayer CaveBackgroundLayers[BackgroundBiomeCount][CaveBackgroundLayerCount] =
	{
		{
			{ L"assets\\back\\bg_grass_cave.png", 1.0f, 0.026f, 0.014f, 5.0f },
			{ L"assets\\back\\bg_grass_cave_far.png", 1.0f, 0.064f, 0.030f, 5.0f },
			{ L"assets\\back\\bg_grass_cave_mid.png", 1.0f, 0.118f, 0.052f, 5.0f },
			{ L"assets\\back\\bg_grass_cave_front.png", 1.0f, 0.200f, 0.082f, 5.0f },
		},
		{
			{ L"assets\\back\\bg_desert_cave.png", 1.0f, 0.026f, 0.014f, 5.0f },
			{ L"assets\\back\\bg_desert_cave_far.png", 1.0f, 0.064f, 0.030f, 5.0f },
			{ L"assets\\back\\bg_desert_cave_mid.png", 1.0f, 0.118f, 0.052f, 5.0f },
			{ L"assets\\back\\bg_desert_cave_front.png", 1.0f, 0.200f, 0.082f, 5.0f },
		},
		{
			{ L"assets\\back\\bg_ice_cave.png", 1.0f, 0.026f, 0.014f, 5.0f },
			{ L"assets\\back\\bg_ice_cave_far.png", 1.0f, 0.064f, 0.030f, 5.0f },
			{ L"assets\\back\\bg_ice_cave_mid.png", 1.0f, 0.118f, 0.052f, 5.0f },
			{ L"assets\\back\\bg_ice_cave_front.png", 1.0f, 0.200f, 0.082f, 5.0f },
		},
	};
	constexpr float MonsterCollisionWidth = 10.0f;
	constexpr float MonsterCollisionHeight = 30.0f;
	constexpr float MonsterMoveSpeed = 48.0f;
	constexpr float PlayerAttackRange = 42.0f;
	constexpr float PlayerAttackHeight = 40.0f;
	constexpr float PlayerAttackDuration = 0.18f;

	enum BlockType : unsigned short
	{
		BlockGrass = 0,
		BlockDirt = 1,
		BlockStone = 2,
		BlockOre = 3,
		BlockSand = 4,
		BlockWood = 5,
		BlockLeaves = 6,
		BlockCraftingTable = 7,
		BlockPrairieStone = 8,
		BlockMossStone = 9,
		BlockSandstone = 10,
		BlockDesertStone = 11,
		BlockSnow = 12,
		BlockIce = 13,
		BlockFrozenStone = 14,
		BlockCrystalOre = 15,
	};

	constexpr int SlotGrass = 0;
	constexpr int SlotDirt = 1;
	constexpr int SlotStone = 2;
	constexpr int SlotOre = 3;
	constexpr int SlotSand = 4;
	constexpr int SlotWood = 5;
	constexpr int SlotLeaves = 6;
	constexpr int SlotCraftingTable = 7;
	constexpr int SlotSword = 8;
	constexpr int SlotAxe = 9;
	constexpr int BlockInventorySlotCount = 8;

	constexpr unsigned short InventoryTileIndices[] =
	{
		BlockGrass,
		BlockDirt,
		BlockStone,
		BlockOre,
		BlockSand,
		BlockWood,
		BlockLeaves,
		BlockCraftingTable,
	};

	enum BiomeType
	{
		BiomeGrassland,
		BiomeDesert,
		BiomeIce,
	};

	bool IsStoneLikeTile(unsigned short tileIndex)
	{
		return tileIndex == BlockStone ||
			tileIndex == BlockPrairieStone ||
			tileIndex == BlockMossStone ||
			tileIndex == BlockSandstone ||
			tileIndex == BlockDesertStone ||
			tileIndex == BlockFrozenStone ||
			tileIndex == BlockIce;
	}

	unsigned short GetTerrainTileForBiome(BiomeType biome, int depth, float layerNoise)
	{
		switch (biome)
		{
		case BiomeDesert:
			if (depth < 7)
				return BlockSand;
			if (depth < 18)
				return layerNoise > 0.62f ? BlockDesertStone : BlockSandstone;
			return layerNoise > 0.55f ? BlockDesertStone : BlockSandstone;
		case BiomeIce:
			if (depth == 0)
				return BlockSnow;
			if (depth < 4)
				return layerNoise > 0.34f ? BlockIce : BlockSnow;
			if (depth < 14)
				return layerNoise > 0.58f ? BlockIce : BlockFrozenStone;
			return layerNoise > 0.72f ? BlockIce : BlockFrozenStone;
		case BiomeGrassland:
		default:
			if (depth == 0)
				return BlockGrass;
			if (depth <= 4)
				return BlockDirt;
			if (depth < 14)
				return layerNoise > 0.56f ? BlockMossStone : BlockPrairieStone;
			return layerNoise > 0.72f ? BlockMossStone : BlockPrairieStone;
		}
	}

	constexpr const WCHAR* PlayerSpriteTexture = L"assets\\char\\player_chibi_sheet.png";
	constexpr int PlayerSpriteColumns = 9;
	constexpr int PlayerSpriteRows = 1;
	constexpr int PlayerIdleFrameStart = 0;
	constexpr int PlayerWalkFrameStart = 4;
	constexpr int PlayerJumpFrame = 8;
	constexpr const WCHAR* MonsterSpriteTextures[BackgroundBiomeCount] =
	{
		L"assets\\monster\\monster_grass_sheet.png",
		L"assets\\monster\\monster_desert_sheet.png",
		L"assets\\monster\\monster_ice_sheet.png",
	};
	constexpr int MonsterSpriteColumns = 4;
	constexpr int MonsterSpriteRows = 1;
	constexpr float MonsterSpriteDrawSize = 48.0f;
	constexpr float MonsterSpriteYOffset = 4.0f;

	struct CrackSegment
	{
		float startX;
		float startY;
		float endX;
		float endY;
		float appearAt;
		float completeAt;
	};

	const CrackSegment CrackSegments[] =
	{
		{ -0.08f,  0.32f,  0.05f,  0.08f, 0.00f, 0.28f },
		{  0.05f,  0.08f, -0.22f, -0.05f, 0.18f, 0.45f },
		{  0.05f,  0.08f,  0.30f, -0.16f, 0.25f, 0.55f },
		{ -0.02f, -0.04f, -0.36f, -0.30f, 0.40f, 0.70f },
		{  0.16f, -0.03f,  0.38f,  0.23f, 0.48f, 0.78f },
		{ -0.17f, -0.02f, -0.36f,  0.16f, 0.58f, 0.86f },
		{  0.03f,  0.15f,  0.27f,  0.37f, 0.66f, 0.95f },
	};

	float Clamp01(float value)
	{
		if (value < 0.0f)
			return 0.0f;
		if (value > 1.0f)
			return 1.0f;
		return value;
	}

	float Lerp(float a, float b, float t)
	{
		return a + (b - a) * t;
	}

	float SmoothStep(float t)
	{
		return t * t * (3.0f - 2.0f * t);
	}

	unsigned int HashUInt(unsigned int value)
	{
		value ^= value >> 16;
		value *= 0x7feb352d;
		value ^= value >> 15;
		value *= 0x846ca68b;
		value ^= value >> 16;
		return value;
	}

	float Hash01(int x, int y, unsigned int seed)
	{
		const unsigned int hx = static_cast<unsigned int>(x) * 374761393u;
		const unsigned int hy = static_cast<unsigned int>(y) * 668265263u;
		return static_cast<float>(HashUInt(hx ^ hy ^ seed) & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
	}

	float ValueNoise1D(float x, unsigned int seed)
	{
		const int ix = static_cast<int>(std::floor(x));
		const float fx = x - static_cast<float>(ix);
		const float a = Hash01(ix, 0, seed);
		const float b = Hash01(ix + 1, 0, seed);
		return Lerp(a, b, SmoothStep(fx));
	}

	float ValueNoise2D(float x, float y, unsigned int seed)
	{
		const int ix = static_cast<int>(std::floor(x));
		const int iy = static_cast<int>(std::floor(y));
		const float fx = x - static_cast<float>(ix);
		const float fy = y - static_cast<float>(iy);
		const float a = Hash01(ix, iy, seed);
		const float b = Hash01(ix + 1, iy, seed);
		const float c = Hash01(ix, iy + 1, seed);
		const float d = Hash01(ix + 1, iy + 1, seed);
		const float u = SmoothStep(fx);
		const float v = SmoothStep(fy);
		return Lerp(Lerp(a, b, u), Lerp(c, d, u), v);
	}

	float FractalNoise1D(float x, unsigned int seed, int octaves, float lacunarity, float gain)
	{
		float value = 0.0f;
		float amplitude = 0.5f;
		float frequency = 1.0f;
		float amplitudeSum = 0.0f;
		for (int octave = 0; octave < octaves; ++octave)
		{
			value += ValueNoise1D(x * frequency, seed + octave * 1013u) * amplitude;
			amplitudeSum += amplitude;
			frequency *= lacunarity;
			amplitude *= gain;
		}

		return amplitudeSum > 0.0f ? value / amplitudeSum : 0.0f;
	}

	float FractalNoise2D(float x, float y, unsigned int seed, int octaves, float lacunarity, float gain)
	{
		float value = 0.0f;
		float amplitude = 0.5f;
		float frequency = 1.0f;
		float amplitudeSum = 0.0f;
		for (int octave = 0; octave < octaves; ++octave)
		{
			value += ValueNoise2D(x * frequency, y * frequency, seed + octave * 1999u) * amplitude;
			amplitudeSum += amplitude;
			frequency *= lacunarity;
			amplitude *= gain;
		}

		return amplitudeSum > 0.0f ? value / amplitudeSum : 0.0f;
	}

	struct GlyphPattern
	{
		char glyph;
		unsigned char rows[7];
	};

	constexpr GlyphPattern Font5x7[] =
	{
		{ '0', { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E } },
		{ '1', { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E } },
		{ '2', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F } },
		{ '3', { 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E } },
		{ '4', { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 } },
		{ '5', { 0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E } },
		{ '6', { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E } },
		{ '7', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 } },
		{ '8', { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E } },
		{ '9', { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C } },
		{ 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
		{ 'B', { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E } },
		{ 'C', { 0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0F } },
		{ 'D', { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E } },
		{ 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
		{ 'F', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 } },
		{ 'G', { 0x0F, 0x10, 0x10, 0x13, 0x11, 0x11, 0x0F } },
		{ 'H', { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
		{ 'I', { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E } },
		{ 'J', { 0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E } },
		{ 'K', { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 } },
		{ 'L', { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } },
		{ 'M', { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 } },
		{ 'N', { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 } },
		{ 'O', { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
		{ 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
		{ 'Q', { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D } },
		{ 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
		{ 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
		{ 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
		{ 'U', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
		{ 'V', { 0x11, 0x11, 0x11, 0x11, 0x0A, 0x0A, 0x04 } },
		{ 'W', { 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A } },
		{ 'X', { 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 } },
		{ 'Y', { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 } },
		{ 'Z', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F } },
		{ ':', { 0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00 } },
		{ '.', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C } },
		{ '-', { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 } },
	};

	const unsigned char* GetGlyphRows(char glyph)
	{
		if (glyph >= 'a' && glyph <= 'z')
			glyph = static_cast<char>(glyph - 'a' + 'A');

		for (const GlyphPattern& pattern : Font5x7)
		{
			if (pattern.glyph == glyph)
				return pattern.rows;
		}

		return nullptr;
	}

	float CounterMilliseconds(LARGE_INTEGER start, LARGE_INTEGER end)
	{
		static LARGE_INTEGER frequency = {};
		if (frequency.QuadPart == 0)
			QueryPerformanceFrequency(&frequency);

		return static_cast<float>(
			(static_cast<double>(end.QuadPart - start.QuadPart) * 1000.0) /
			static_cast<double>(frequency.QuadPart));
	}
}

void GameEngine::Start(const char* szTitle, float x, float y, float width, float height, HINSTANCE hInstance)
{
	m_timerResolutionRaised = timeBeginPeriod(1) == TIMERR_NOERROR;

	CreateRenderer(&m_renderer);

	CreateWindowInstance(&m_window);
	m_window->Initialize(szTitle, x, y, width, height, hInstance, this);
	m_targetRefreshRate = GetMonitorRefreshRate();

	m_renderer->Initialize((UINT)m_window->GetWidth(), (UINT)m_window->GetHeight(), *m_window->GetHWND());

	CreateTimer(&m_timer);
	m_timer->Initialize();

	CreateInput(&m_input);
	m_input->Initialize();
	m_input->AddUser(this);

	m_renderer->LoadTexture(BlockAtlasTexture);
	for (const auto& biomeLayers : SurfaceBackgroundLayers)
	{
		for (const BackgroundImageLayer& layer : biomeLayers)
			m_renderer->LoadTexture(layer.textureFile);
	}
	for (const auto& biomeLayers : CaveBackgroundLayers)
	{
		for (const BackgroundImageLayer& layer : biomeLayers)
			m_renderer->LoadTexture(layer.textureFile);
	}
	m_renderer->LoadTexture(PlayerSpriteTexture);
	for (const WCHAR* monsterTexture : MonsterSpriteTextures)
		m_renderer->LoadTexture(monsterTexture);

	InitializeWorld();

	m_timer->Reset();
	m_window->RunMessageLoop();
}

void GameEngine::Update()
{
	LARGE_INTEGER frameStart = {};
	LARGE_INTEGER sectionStart = {};
	LARGE_INTEGER sectionEnd = {};
	QueryPerformanceCounter(&frameStart);
	sectionStart = frameStart;
	CpuFrameStats cpuStats;

	float deltaTime = m_timer->GetElapsedSeconds();
	m_timer->Reset();
	if (deltaTime > 0.05f)
		deltaTime = 0.05f;

	UpdateFrameStats(deltaTime);
	m_input->Update();
	m_uiConsumesLeftMouse = false;
	if (m_input->IsDown(KeyDebugStats, this))
		m_showRenderStats = !m_showRenderStats;
	if (m_input->IsDown(KeyFrameLimit, this))
		ToggleFrameLimiter();
	if (m_input->IsDown(KeyMinimap, this))
	{
		m_minimapExpanded = !m_minimapExpanded;
		m_minimapDirty = true;
	}
	if (m_input->IsDown(KeyRevealMap, this))
		RevealAllMap();

	QueryPerformanceCounter(&sectionEnd);
	cpuStats.inputMs = CounterMilliseconds(sectionStart, sectionEnd);
	sectionStart = sectionEnd;

	UpdateInventoryInput();
	UpdateCrafting(deltaTime);
	TryPlaceSelectedBlock();
	UpdateBlockBreaking(deltaTime);
	UpdatePlayer(deltaTime);
	UpdateMapReveal();
	UpdatePlayerCombat(deltaTime);

	QueryPerformanceCounter(&sectionEnd);
	cpuStats.simulationMs = CounterMilliseconds(sectionStart, sectionEnd);
	sectionStart = sectionEnd;

	UpdateMonsters(deltaTime);

	QueryPerformanceCounter(&sectionEnd);
	cpuStats.monstersMs = CounterMilliseconds(sectionStart, sectionEnd);
	sectionStart = sectionEnd;

	UpdateDroppedItems(deltaTime);

	QueryPerformanceCounter(&sectionEnd);
	cpuStats.itemsMs = CounterMilliseconds(sectionStart, sectionEnd);
	sectionStart = sectionEnd;

	m_renderer->BeginFrame();
	LARGE_INTEGER drawStart = {};
	LARGE_INTEGER drawEnd = {};
	QueryPerformanceCounter(&drawStart);
	DrawWorld();
	QueryPerformanceCounter(&drawEnd);
	m_renderer->EndFrame();

	QueryPerformanceCounter(&sectionEnd);
	cpuStats.drawWorldMs = CounterMilliseconds(drawStart, drawEnd);
	cpuStats.renderMs = CounterMilliseconds(sectionStart, sectionEnd);
	cpuStats.totalMs = CounterMilliseconds(frameStart, sectionEnd);
	AccumulateCpuStats(cpuStats);
	ApplyFrameLimiter(frameStart);
}

void GameEngine::InitializeWorld()
{
	m_blocks.assign(m_blockWidth * m_blockHeight, BlockTile{});
	m_blockBreaks.assign(m_blockWidth * m_blockHeight, BlockBreakState{});
	m_surfaceHeights.clear();
	m_biomes.clear();
	m_revealedTiles.assign(static_cast<size_t>(m_blockWidth * m_blockHeight), 0);
	m_monsters.clear();
	m_droppedItems.clear();
	m_monsterQueryScratch.clear();
	m_monsterOverlapScratch.clear();
	m_minimapRuns.clear();
	m_minimapDirty = true;
	m_debugRevealMap = false;
	m_minimapExpanded = false;
	m_cachedMinimapSampleStep = -1;
	m_worldOriginX = -((m_blockWidth - 1) * m_tileSize) * 0.5f;

	const unsigned int seed = GetTickCount();
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
	m_surfaceHeights = surfaceHeights;
	m_biomes.resize(m_blockWidth);
	for (int x = 0; x < m_blockWidth; ++x)
		m_biomes[x] = static_cast<unsigned char>(biomes[x]);

	for (int y = 0; y < m_blockHeight; ++y)
	{
		for (int x = 0; x < m_blockWidth; ++x)
		{
			BlockTile& tile = m_blocks[y * m_blockWidth + x];
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
				m_blocks[y * m_blockWidth + x].visible = 0;
		}
	}

	auto restoreTerrainTile = [this, &surfaceHeights, &biomes, seed](int tileX, int tileY)
	{
		if (!IsTileInBounds(tileX, tileY) || tileY < surfaceHeights[tileX])
			return;

		BlockTile& tile = m_blocks[tileY * m_blockWidth + tileX];
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
					if (m_blocks[blockIndex].visible != 0 || y <= surfaceHeights[x] + 10)
						continue;

					int solidNeighbors = 0;
					for (int oy = -1; oy <= 1; ++oy)
					{
						for (int ox = -1; ox <= 1; ++ox)
						{
							if (ox == 0 && oy == 0)
								continue;

							const int neighborIndex = (y + oy) * m_blockWidth + (x + ox);
							solidNeighbors += m_blocks[neighborIndex].visible != 0 ? 1 : 0;
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
					m_blocks[y * m_blockWidth + x].visible = 0;
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
					m_blocks[y * m_blockWidth + tileX].visible = 0;
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
			if (connected[index] != 0 || m_blocks[index].visible == 0)
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
			if (visited[index] != 0 || m_blocks[index].visible == 0)
				return;

			visited[index] = 1;
			queue.push_back(index);
		};

		for (int index = 0; index < tileCount; ++index)
		{
			if (visited[index] != 0 || m_blocks[index].visible == 0)
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
					m_blocks[detachedIndex].visible = 0;
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

					BlockTile& tile = m_blocks[y * m_blockWidth + x];
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

		BlockTile& tile = m_blocks[tileY * m_blockWidth + tileX];
		if (tile.visible != 0)
			return;

		tile.visible = 1;
		tile.tileIndex = tileIndex;
	};

	auto isTreeTile = [this](int tileX, int tileY)
	{
		if (!IsTileInBounds(tileX, tileY))
			return false;

		const BlockTile& tile = m_blocks[tileY * m_blockWidth + tileX];
		return tile.visible != 0 && (tile.tileIndex == BlockWood || tile.tileIndex == BlockLeaves);
	};

	auto addLeafAnchor = [this](std::vector<int>& leafAnchors, int tileX, int tileY)
	{
		if (!IsTileInBounds(tileX, tileY))
			return;

		const int anchorIndex = tileY * m_blockWidth + tileX;
		const BlockTile& tile = m_blocks[anchorIndex];
		if (tile.visible != 0 && tile.tileIndex == BlockWood)
			leafAnchors.push_back(anchorIndex);
	};

	auto placeConnectedCanopy = [&](const std::vector<int>& leafAnchors, BiomeType biome)
	{
		for (int anchorIndex : leafAnchors)
		{
			const int anchorX = anchorIndex % m_blockWidth;
			const int anchorY = anchorIndex / m_blockWidth;
			const int radius = biome == BiomeIce ? randomInt(2, 3) : randomInt(2, 4);
			const int topReach = radius;
			const int bottomReach = biome == BiomeIce ? 1 : randomInt(1, 2);

			for (int leafY = anchorY - topReach; leafY <= anchorY + bottomReach; ++leafY)
			{
				for (int leafX = anchorX - radius; leafX <= anchorX + radius; ++leafX)
				{
					const int dx = std::abs(leafX - anchorX);
					const int dy = std::abs(leafY - anchorY);
					const int allowedWidth = radius - (dy > 1 ? dy - 1 : 0);
					if (allowedWidth < 0 || dx > allowedWidth)
						continue;

					setSkyBlock(leafX, leafY, BlockLeaves);
				}
			}
		}

		for (int anchorIndex : leafAnchors)
		{
			const int anchorX = anchorIndex % m_blockWidth;
			const int anchorY = anchorIndex / m_blockWidth;
			const int fringeRadius = biome == BiomeIce ? 3 : 4;

			for (int leafY = anchorY - fringeRadius; leafY <= anchorY + 2; ++leafY)
			{
				for (int leafX = anchorX - fringeRadius; leafX <= anchorX + fringeRadius; ++leafX)
				{
					if (!IsTileInBounds(leafX, leafY))
						continue;

					BlockTile& tile = m_blocks[leafY * m_blockWidth + leafX];
					if (tile.visible != 0)
						continue;

					const int distance = std::abs(leafX - anchorX) + std::abs(leafY - anchorY);
					if (distance > fringeRadius + 1 || randomInt(0, 99) < 62)
						continue;

					if (isTreeTile(leafX - 1, leafY) || isTreeTile(leafX + 1, leafY) ||
						isTreeTile(leafX, leafY - 1) || isTreeTile(leafX, leafY + 1))
					{
						tile.visible = 1;
						tile.tileIndex = BlockLeaves;
					}
				}
			}
		}
	};

	int nextTreeX = randomInt(2, 5);
	for (int x = 2; x < m_blockWidth - 2; ++x)
	{
		const BiomeType biome = biomes[x];
		if (x < nextTreeX)
			continue;

		const int surfaceY = surfaceHeights[x];
		if (surfaceY < 7 || surfaceY > m_blockHeight - 8)
			continue;

		const int treeChance = biome == BiomeGrassland ? 68 : (biome == BiomeDesert ? 12 : 26);
		if (randomInt(0, 99) > treeChance)
			continue;

		const int trunkHeight = biome == BiomeDesert ? randomInt(3, 8) : (biome == BiomeIce ? randomInt(5, 12) : randomInt(4, 11));
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

	for (int y = 0; y < spawnSurfaceY; ++y)
	{
		for (int x = spawnX - 3; x <= spawnX + 3; ++x)
		{
			if (IsTileInBounds(x, y))
				m_blocks[y * m_blockWidth + x].visible = 0;
		}
	}
	for (int x = spawnX - spawnSafeRadius; x <= spawnX + spawnSafeRadius; ++x)
	{
		if (x < 0 || x >= m_blockWidth)
			continue;

		for (int y = spawnSurfaceY; y <= spawnSurfaceY + 6 && y < m_blockHeight; ++y)
		{
			BlockTile& tile = m_blocks[y * m_blockWidth + x];
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
		BlockTile& tableTile = m_blocks[tableY * m_blockWidth + tableX];
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
		monster.biome = m_biomes.empty() ? static_cast<unsigned char>(BiomeGrassland) :
			static_cast<unsigned char>(std::clamp(static_cast<int>(m_biomes[tileX]), 0, BackgroundBiomeCount - 1));
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

	m_inventoryCounts = { 12, 48, 48, 8, 48, 24, 32, 1, 0, 0 };
	m_selectedInventorySlot = 0;
	m_playerHealth = 100;
	m_playerInvulnerableTimer = 0.0f;
	m_attackCooldown = 0.0f;
	m_attackTimer = 0.0f;
	SetStatusText("C NEAR TABLE MAKES TOOLS", 2.6f);

	const float groundTop = TileTop(spawnSurfaceY);
	m_player.x = m_worldOriginX + spawnX * m_tileSize;
	m_player.y = groundTop + (m_playerCollisionHeight * 0.5f) + 0.02f;
	m_playerSpawnX = m_player.x;
	m_playerSpawnY = m_player.y;
	m_player.velocityY = 0.0f;
	m_player.animationTime = 0.0f;
	m_player.facing = 1;
	m_player.onGround = true;
	m_cameraX = m_player.x;
	m_cameraY = m_player.y;
	UpdateMapReveal();
}

void GameEngine::UpdateFrameStats(float deltaTime)
{
	m_frameStatsTimer += deltaTime;
	++m_frameStatsCount;

	if (m_frameStatsTimer < 0.25f)
		return;

	m_displayFps = static_cast<float>(m_frameStatsCount) / m_frameStatsTimer;
	m_displayFrameMs = (m_frameStatsTimer / static_cast<float>(m_frameStatsCount)) * 1000.0f;
	m_frameStatsTimer = 0.0f;
	m_frameStatsCount = 0;
}

void GameEngine::AccumulateCpuStats(const CpuFrameStats& stats)
{
	m_cpuStatsAccum.totalMs += stats.totalMs;
	m_cpuStatsAccum.inputMs += stats.inputMs;
	m_cpuStatsAccum.simulationMs += stats.simulationMs;
	m_cpuStatsAccum.monstersMs += stats.monstersMs;
	m_cpuStatsAccum.itemsMs += stats.itemsMs;
	m_cpuStatsAccum.renderMs += stats.renderMs;
	m_cpuStatsAccum.drawWorldMs += stats.drawWorldMs;
	++m_cpuStatsCount;

	if (m_cpuStatsAccum.totalMs < 250.0f && m_cpuStatsCount < 240)
		return;

	const float divisor = static_cast<float>((std::max)(1, m_cpuStatsCount));
	m_displayCpuStats = m_cpuStatsAccum;
	m_displayCpuStats.totalMs /= divisor;
	m_displayCpuStats.inputMs /= divisor;
	m_displayCpuStats.simulationMs /= divisor;
	m_displayCpuStats.monstersMs /= divisor;
	m_displayCpuStats.itemsMs /= divisor;
	m_displayCpuStats.renderMs /= divisor;
	m_displayCpuStats.drawWorldMs /= divisor;
	m_cpuStatsAccum = CpuFrameStats();
	m_cpuStatsCount = 0;
}

void GameEngine::ToggleFrameLimiter()
{
	m_targetRefreshRate = GetMonitorRefreshRate();
	m_frameLimitEnabled = !m_frameLimitEnabled;

	char text[48] = {};
	if (m_frameLimitEnabled)
		std::snprintf(text, sizeof(text), "FPS LIMIT %dHZ", m_targetRefreshRate);
	else
		std::snprintf(text, sizeof(text), "FPS LIMIT OFF");

	SetStatusText(text, 1.6f);
}

void GameEngine::ApplyFrameLimiter(const LARGE_INTEGER& frameStart)
{
	if (!m_frameLimitEnabled)
		return;

	const int refreshRate = (std::max)(30, m_targetRefreshRate);
	const double targetFrameMs = 1000.0 / static_cast<double>(refreshRate);

	while (true)
	{
		LARGE_INTEGER now = {};
		QueryPerformanceCounter(&now);
		const double elapsedMs = static_cast<double>(CounterMilliseconds(frameStart, now));
		const double remainingMs = targetFrameMs - elapsedMs;
		if (remainingMs <= 0.0)
			break;

		if (remainingMs > 1.5)
			Sleep(static_cast<DWORD>((std::max)(1.0, remainingMs - 0.75)));
		else if (remainingMs > 0.25)
			SwitchToThread();
		else
			YieldProcessor();
	}
}

int GameEngine::GetMonitorRefreshRate() const
{
	HWND hwnd = nullptr;
	if (m_window != nullptr)
	{
		HWND* windowHandle = m_window->GetHWND();
		if (windowHandle != nullptr)
			hwnd = *windowHandle;
	}

	HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	if (monitor != nullptr)
	{
		MONITORINFOEXA monitorInfo = {};
		monitorInfo.cbSize = sizeof(monitorInfo);
		if (GetMonitorInfoA(monitor, reinterpret_cast<MONITORINFO*>(&monitorInfo)))
		{
			DEVMODEA displayMode = {};
			displayMode.dmSize = sizeof(displayMode);
			if (EnumDisplaySettingsA(monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &displayMode) &&
				displayMode.dmDisplayFrequency >= 30 &&
				displayMode.dmDisplayFrequency <= 1000)
			{
				return static_cast<int>(displayMode.dmDisplayFrequency);
			}
		}
	}

	DEVMODEA displayMode = {};
	displayMode.dmSize = sizeof(displayMode);
	if (EnumDisplaySettingsA(nullptr, ENUM_CURRENT_SETTINGS, &displayMode) &&
		displayMode.dmDisplayFrequency >= 30 &&
		displayMode.dmDisplayFrequency <= 1000)
	{
		return static_cast<int>(displayMode.dmDisplayFrequency);
	}

	return 60;
}

void GameEngine::UpdateMapReveal()
{
	if (m_debugRevealMap || m_revealedTiles.size() != m_blocks.size())
		return;

	const int centerTileX = std::clamp(WorldToTileX(m_player.x), 0, m_blockWidth - 1);
	const int centerTileY = std::clamp(WorldToTileY(m_player.y), 0, m_blockHeight - 1);
	const int radiusX = (std::max)(18, static_cast<int>(std::ceil(GetViewHalfWidth() / m_tileSize)) + 5);
	const int radiusY = (std::max)(12, static_cast<int>(std::ceil(GetViewHalfHeight() / m_tileSize)) + 5);
	const int startX = (std::max)(0, centerTileX - radiusX);
	const int endX = (std::min)(m_blockWidth - 1, centerTileX + radiusX);
	const int startY = (std::max)(0, centerTileY - radiusY);
	const int endY = (std::min)(m_blockHeight - 1, centerTileY + radiusY);

	bool changed = false;
	for (int y = startY; y <= endY; ++y)
	{
		const float normalizedY = static_cast<float>(y - centerTileY) / static_cast<float>(radiusY);
		for (int x = startX; x <= endX; ++x)
		{
			const float normalizedX = static_cast<float>(x - centerTileX) / static_cast<float>(radiusX);
			if (normalizedX * normalizedX + normalizedY * normalizedY > 1.0f)
				continue;

			const int index = y * m_blockWidth + x;
			if (m_revealedTiles[index] != 0)
				continue;

			m_revealedTiles[index] = 1;
			changed = true;
		}
	}

	if (changed)
		m_minimapDirty = true;
}

void GameEngine::RevealAllMap()
{
	if (m_revealedTiles.size() != m_blocks.size())
		m_revealedTiles.assign(m_blocks.size(), 1);
	else
		std::fill(m_revealedTiles.begin(), m_revealedTiles.end(), 1);

	m_debugRevealMap = true;
	m_minimapExpanded = true;
	m_minimapDirty = true;
	SetStatusText("MAP REVEALED", 1.6f);
}

void GameEngine::UpdateInventoryInput()
{
	if (m_window != nullptr)
	{
		m_inventoryWheelRemainder += m_window->ConsumeMouseWheelDelta();
		const int wheelSteps = m_inventoryWheelRemainder / WHEEL_DELTA;
		if (wheelSteps != 0)
		{
			m_inventoryWheelRemainder -= wheelSteps * WHEEL_DELTA;
			m_selectedInventorySlot -= wheelSteps;
			m_selectedInventorySlot %= InventorySlotCount;
			if (m_selectedInventorySlot < 0)
				m_selectedInventorySlot += InventorySlotCount;
		}
	}

	for (int slot = 0; slot < InventorySlotCount; ++slot)
	{
		const int keyCode = slot < 9 ? 0x31 + slot : 0x30;
		if (m_input->IsDown(keyCode, this))
			m_selectedInventorySlot = slot;
	}
}

bool GameEngine::CraftSword()
{
	if (m_inventoryCounts[SlotSword] > 0)
	{
		m_selectedInventorySlot = SlotSword;
		SetStatusText("SWORD EQUIPPED", 1.2f);
		return true;
	}

	if (!CanCraftSword())
	{
		SetStatusText("NEED 2 WOOD 3 STONE", 1.5f);
		return false;
	}

	m_inventoryCounts[SlotWood] -= 2;
	m_inventoryCounts[SlotStone] -= 3;
	m_inventoryCounts[SlotSword] = 1;
	m_selectedInventorySlot = SlotSword;
	SetStatusText("SWORD CRAFTED", 1.8f);
	return true;
}

bool GameEngine::CraftAxe()
{
	if (m_inventoryCounts[SlotAxe] > 0)
	{
		m_selectedInventorySlot = SlotAxe;
		SetStatusText("AXE EQUIPPED", 1.2f);
		return true;
	}

	if (!CanCraftAxe())
	{
		SetStatusText("NEED 3 WOOD 2 STONE", 1.5f);
		return false;
	}

	m_inventoryCounts[SlotWood] -= 3;
	m_inventoryCounts[SlotStone] -= 2;
	m_inventoryCounts[SlotAxe] = 1;
	m_selectedInventorySlot = SlotAxe;
	SetStatusText("AXE CRAFTED", 1.8f);
	return true;
}

bool GameEngine::CraftFirstAvailableAtTable()
{
	if (m_inventoryCounts[SlotSword] <= 0 && CanCraftSword())
		return CraftSword();
	if (m_inventoryCounts[SlotAxe] <= 0 && CanCraftAxe())
		return CraftAxe();
	if (m_inventoryCounts[SlotSword] <= 0)
		return CraftSword();
	if (m_inventoryCounts[SlotAxe] <= 0)
		return CraftAxe();
	if (m_selectedInventorySlot != SlotSword)
		return CraftSword();
	if (m_selectedInventorySlot != SlotAxe)
		return CraftAxe();

	SetStatusText("TOOLS READY", 1.3f);
	return false;
}

void GameEngine::UpdateCrafting(float deltaTime)
{
	if (m_statusTextTimer > 0.0f)
	{
		m_statusTextTimer -= deltaTime;
		if (m_statusTextTimer <= 0.0f)
		{
			m_statusTextTimer = 0.0f;
			m_statusText[0] = '\0';
		}
	}

	if (m_input == nullptr)
		return;

	if (IsCraftingTableNearby())
	{
		if (IsCursorOverCraftingPanel() && IsKeyHeld(VK_LBUTTON))
			m_uiConsumesLeftMouse = true;

		if (m_input->IsDown(VK_LBUTTON, this))
		{
			float cursorX = 0.0f;
			float cursorY = 0.0f;
			if (GetCursorViewPosition(cursorX, cursorY))
			{
				const int recipe = GetCraftingRecipeAt(cursorX, cursorY);
				if (recipe >= 0)
				{
					m_uiConsumesLeftMouse = true;
					if (recipe == 0)
						CraftSword();
					else if (recipe == 1)
						CraftAxe();

					return;
				}
			}
		}

		if (m_input->IsDown(KeyC, this))
		{
			CraftFirstAvailableAtTable();
			return;
		}

		return;
	}

	if (!m_input->IsDown(KeyC, this))
		return;

	if (m_inventoryCounts[SlotWood] >= 4)
	{
		m_inventoryCounts[SlotWood] -= 4;
		++m_inventoryCounts[SlotCraftingTable];
		m_selectedInventorySlot = SlotCraftingTable;
		SetStatusText("TABLE CRAFTED", 1.8f);
		return;
	}

	SetStatusText("NEED TABLE OR 4 WOOD", 1.8f);
}

void GameEngine::TryPlaceSelectedBlock()
{
	if (m_input == nullptr || !m_input->IsDown(VK_RBUTTON, this))
		return;
	if (IsCursorOverCraftingPanel())
		return;

	int tileX = 0;
	int tileY = 0;
	if (!GetHoveredTile(tileX, tileY) || !CanPlaceSelectedBlockAt(tileX, tileY))
		return;

	const int blockIndex = tileY * m_blockWidth + tileX;
	m_blocks[blockIndex].visible = 1;
	m_blocks[blockIndex].tileIndex = InventoryTileIndices[m_selectedInventorySlot];
	m_blockBreaks[blockIndex] = BlockBreakState{};
	MarkBlockChunkDirty(tileX, tileY);
	--m_inventoryCounts[m_selectedInventorySlot];
}

void GameEngine::UpdatePlayer(float deltaTime)
{
	const float moveSpeed = m_tileSize * 6.8f;
	const float jumpSpeed = m_tileSize * 14.6f;
	const float gravity = -m_tileSize * 36.0f;
	const float maxFallSpeed = -m_tileSize * 26.0f;
	const float collisionInset = 0.5f;
	const float skin = 0.02f;

	float move = 0.0f;
	if (IsKeyHeld(KeyA))
		move -= 1.0f;
	if (IsKeyHeld(KeyD))
		move += 1.0f;

	if (move < 0.0f)
		m_player.facing = -1;
	else if (move > 0.0f)
		m_player.facing = 1;

	const float deltaX = move * moveSpeed * deltaTime;
	const float nextX = m_player.x + deltaX;
	if (!IsAABBBlocked(GetPlayerAABB(nextX, m_player.y), collisionInset))
	{
		m_player.x = nextX;
	}
	else if (deltaX > 0.0f)
	{
		const int tileX = WorldToTileX(collib::Right(GetPlayerAABB(nextX, m_player.y)));
		m_player.x = TileLeft(tileX) - m_playerCollisionWidth * 0.5f - skin;
	}
	else if (deltaX < 0.0f)
	{
		const int tileX = WorldToTileX(collib::Left(GetPlayerAABB(nextX, m_player.y)));
		m_player.x = TileRight(tileX) + m_playerCollisionWidth * 0.5f + skin;
	}

	if (m_input->IsDown(KeyJump, this) && m_player.onGround)
	{
		m_player.velocityY = jumpSpeed;
		m_player.onGround = false;
	}
	else if (m_player.onGround)
	{
		if (IsGroundBelowPlayer(m_player.x, m_player.y))
		{
			m_player.velocityY = 0.0f;
		}
		else
		{
			m_player.onGround = false;
		}
	}

	if (!m_player.onGround)
	{
		float frameGravity = gravity;
		if (IsKeyHeld(KeyS) && m_player.velocityY < 0.0f)
			frameGravity *= 1.8f;

		m_player.velocityY += frameGravity * deltaTime;
		if (m_player.velocityY < maxFallSpeed)
			m_player.velocityY = maxFallSpeed;

		const float deltaY = m_player.velocityY * deltaTime;
		const float nextY = m_player.y + deltaY;
		if (!IsAABBBlocked(GetPlayerAABB(m_player.x, nextY), collisionInset))
		{
			m_player.y = nextY;
			m_player.onGround = false;
		}
		else if (deltaY < 0.0f)
		{
			const int tileY = WorldToTileY(collib::Bottom(GetPlayerAABB(m_player.x, nextY)));
			m_player.y = TileTop(tileY) + m_playerCollisionHeight * 0.5f + skin;
			m_player.velocityY = 0.0f;
			m_player.onGround = true;
		}
		else if (deltaY > 0.0f)
		{
			const int tileY = WorldToTileY(collib::Top(GetPlayerAABB(m_player.x, nextY)));
			m_player.y = TileBottom(tileY) - m_playerCollisionHeight * 0.5f - skin;
			m_player.velocityY = 0.0f;
			m_player.onGround = false;
		}
	}

	m_player.animationTime += deltaTime;
	const float cameraFollow = deltaTime * 7.5f;
	const float clampedCameraFollow = cameraFollow > 1.0f ? 1.0f : cameraFollow;
	m_cameraX += (m_player.x - m_cameraX) * clampedCameraFollow;
	m_cameraY += (m_player.y - m_cameraY) * clampedCameraFollow;
}

void GameEngine::UpdatePlayerCombat(float deltaTime)
{
	if (m_attackCooldown > 0.0f)
		m_attackCooldown -= deltaTime;
	if (m_attackTimer > 0.0f)
		m_attackTimer -= deltaTime;
	if (m_playerInvulnerableTimer > 0.0f)
		m_playerInvulnerableTimer -= deltaTime;

	if (m_playerHealth <= 0)
	{
		m_player.x = m_playerSpawnX;
		m_player.y = m_playerSpawnY;
		m_player.velocityY = 0.0f;
		m_player.onGround = true;
		m_cameraX = m_player.x;
		m_cameraY = m_player.y;
		m_playerHealth = 100;
		m_playerInvulnerableTimer = 1.5f;
		SetStatusText("RESPAWNED", 1.8f);
	}

	TryPlayerAttack();
}

void GameEngine::TryPlayerAttack()
{
	if (m_uiConsumesLeftMouse || m_input == nullptr || !m_input->IsDown(VK_LBUTTON, this))
		return;

	if (!ShouldLeftClickAttack())
		return;

	if (m_attackCooldown > 0.0f)
		return;

	m_attackCooldown = 0.34f;
	m_attackTimer = PlayerAttackDuration;
	const bool swordEquipped = m_selectedInventorySlot == SlotSword && m_inventoryCounts[SlotSword] > 0;
	const bool axeEquipped = m_selectedInventorySlot == SlotAxe && m_inventoryCounts[SlotAxe] > 0;
	const int attackDamage = swordEquipped ? 36 : (axeEquipped ? 12 : 8);

	const float attackCenterX = m_player.x + m_player.facing * (PlayerAttackRange * 0.48f);
	const collib::AABB attackBox = collib::MakeAABB(
		attackCenterX,
		m_player.y,
		PlayerAttackRange,
		PlayerAttackHeight);

	bool hit = false;
	QueryMonstersInAABB(attackBox, m_monsterQueryScratch);
	for (int monsterIndex : m_monsterQueryScratch)
	{
		MonsterState& monster = m_monsters[monsterIndex];
		if (!monster.alive)
			continue;

		monster.health -= attackDamage;
		monster.hurtTimer = 0.16f;
		monster.velocityY = (std::max)(monster.velocityY, swordEquipped ? 170.0f : (axeEquipped ? 105.0f : 90.0f));
		monster.x += m_player.facing * (swordEquipped ? 22.0f : (axeEquipped ? 11.0f : 9.0f));
		monster.facing = -m_player.facing;
		hit = true;

		if (monster.health <= 0)
		{
			monster.alive = false;
			SpawnDroppedItem(monster.x, monster.y, BlockOre, 1);
		}
	}

	if (hit)
	{
		SetStatusText("HIT", 0.5f);
	}
}

void GameEngine::UpdateMonsters(float deltaTime)
{
	const float gravity = -m_tileSize * 34.0f;
	const float maxFallSpeed = -m_tileSize * 24.0f;
	const float collisionInset = 0.5f;
	const float skin = 0.02f;

	for (MonsterState& monster : m_monsters)
	{
		if (!monster.alive)
			continue;

		if (monster.hurtTimer > 0.0f)
			monster.hurtTimer -= deltaTime;
		if (monster.attackCooldown > 0.0f)
			monster.attackCooldown -= deltaTime;
		if (monster.jumpCooldown > 0.0f)
			monster.jumpCooldown -= deltaTime;
		if (monster.idleTimer > 0.0f)
			monster.idleTimer -= deltaTime;
		if (monster.aiTimer > 0.0f)
			monster.aiTimer -= deltaTime;
		if (monster.contactTimer > 0.0f)
		{
			monster.contactTimer -= deltaTime;
			if (monster.contactTimer <= 0.0f)
			{
				monster.contactTimer = 0.0f;
				monster.contactDirection = 0;
			}
		}

		const float playerDeltaX = m_player.x - monster.x;
		const float playerDeltaY = m_player.y - monster.y;
		const float playerDistanceX = std::fabs(playerDeltaX);
		const float playerDistanceY = std::fabs(playerDeltaY);
		const float awarenessX = monster.underground ? m_tileSize * 24.0f : m_tileSize * 18.0f;
		const float awarenessY = monster.underground ? m_tileSize * 11.0f : m_tileSize * 7.0f;
		const bool chasingPlayer = playerDistanceX < awarenessX && playerDistanceY < awarenessY;
		const bool touchingPlayer = collib::Intersects(GetPlayerAABB(m_player.x, m_player.y), GetMonsterAABB(monster.x, monster.y));
		if (touchingPlayer && monster.contactTimer <= 0.0f)
		{
			const int tileX = WorldToTileX(monster.x);
			const int tileY = WorldToTileY(monster.y);
			if (playerDistanceX > MonsterCollisionWidth * 0.35f)
				monster.contactDirection = monster.x < m_player.x ? 1 : -1;
			else
				monster.contactDirection = Hash01(tileX, tileY, GetTickCount()) > 0.5f ? 1 : -1;

			monster.contactTimer = 0.36f + Hash01(tileX + 9, tileY - 3, GetTickCount() + 61u) * 0.24f;
		}

		if (chasingPlayer)
		{
			if (monster.contactTimer > 0.0f && monster.contactDirection != 0)
				monster.facing = monster.contactDirection;
			else if (playerDistanceX > MonsterCollisionWidth * 0.35f)
				monster.facing = playerDeltaX >= 0.0f ? 1 : -1;
			monster.idleTimer = 0.0f;
		}
		else
		{
			const float leashDistance = monster.underground ? m_tileSize * 13.0f : m_tileSize * 9.0f;
			if (std::fabs(monster.x - monster.homeX) > leashDistance)
			{
				monster.facing = monster.x < monster.homeX ? 1 : -1;
				monster.idleTimer = 0.0f;
			}
			else if (monster.aiTimer <= 0.0f)
			{
				const int tileX = WorldToTileX(monster.x);
				const int tileY = WorldToTileY(monster.y);
				const float choice = Hash01(tileX, tileY, GetTickCount());
				monster.aiTimer = 0.65f + Hash01(tileX + 17, tileY - 9, GetTickCount() + 37u) * 1.45f;
				if (choice < 0.22f)
					monster.idleTimer = 0.35f + choice * 1.9f;
				else if (choice > 0.64f)
					monster.facing *= -1;
			}
		}

		const bool contactShuffle = monster.contactTimer > 0.0f && monster.contactDirection != 0;
		if (contactShuffle)
			monster.facing = monster.contactDirection;

		const bool idle = monster.idleTimer > 0.0f && !chasingPlayer && !contactShuffle;
		float speedMultiplier = chasingPlayer ? (monster.underground ? 1.28f : 1.12f) : (monster.underground ? 0.82f : 0.68f);
		if (contactShuffle)
			speedMultiplier = monster.underground ? 1.42f : 1.24f;
		const float previousX = monster.x;
		const float deltaX = idle ? 0.0f : monster.facing * MonsterMoveSpeed * speedMultiplier * deltaTime;
		const float nextX = monster.x + deltaX;
		const int groundProbeX = WorldToTileX(nextX + monster.facing * MonsterCollisionWidth * 0.55f);
		const int groundProbeY = WorldToTileY(monster.y - MonsterCollisionHeight * 0.5f - m_tileSize * 0.35f);
		const bool hasGroundAhead = IsSolidTile(groundProbeX, groundProbeY);
		const bool blockedAhead = deltaX != 0.0f && IsAABBBlocked(GetMonsterAABB(nextX, monster.y), collisionInset);

		if (deltaX != 0.0f && !blockedAhead && (hasGroundAhead || chasingPlayer || monster.underground))
		{
			monster.x = nextX;
		}
		else if (deltaX != 0.0f)
		{
			const bool shouldJump = monster.onGround && monster.jumpCooldown <= 0.0f && (blockedAhead || (chasingPlayer && !hasGroundAhead && monster.underground));
			if (shouldJump)
			{
				monster.velocityY = m_tileSize * (monster.underground ? 9.9f : 8.9f);
				monster.onGround = false;
				monster.jumpCooldown = monster.underground ? 0.52f : 0.72f;
			}
			else if (!chasingPlayer || !monster.underground)
			{
				if (contactShuffle)
				{
					monster.contactDirection *= -1;
					monster.facing = monster.contactDirection;
					monster.contactTimer = 0.24f;
				}
				else
				{
					monster.facing *= -1;
				}
				monster.aiTimer = 0.35f;
			}
		}

		if (IsGroundBelowBox(monster.x, monster.y, MonsterCollisionWidth, MonsterCollisionHeight))
		{
			if (monster.velocityY <= 0.0f)
			{
				monster.velocityY = 0.0f;
				monster.onGround = true;
			}
		}
		else
		{
			monster.onGround = false;
		}

		const float actualDeltaX = monster.x - previousX;
		if (std::fabs(monster.x - monster.lastX) < 0.04f && std::fabs(deltaX) > 0.0f)
			monster.stuckTimer += deltaTime;
		else
			monster.stuckTimer = 0.0f;

		if (monster.stuckTimer > 0.45f && monster.onGround && monster.jumpCooldown <= 0.0f)
		{
			monster.velocityY = m_tileSize * 9.3f;
			monster.onGround = false;
			monster.jumpCooldown = 0.65f;
			if (contactShuffle)
			{
				monster.contactDirection *= -1;
				monster.facing = monster.contactDirection;
				monster.contactTimer = 0.30f;
			}
			monster.stuckTimer = 0.0f;
		}
		monster.lastX = monster.x;
		if (std::fabs(actualDeltaX) > 0.015f || touchingPlayer)
			monster.animationTime += deltaTime * (touchingPlayer ? 7.8f : 6.2f);
		else
			monster.animationTime += deltaTime * 1.2f;

		if (!monster.onGround || monster.velocityY > 0.0f)
		{
			monster.velocityY += gravity * deltaTime;
			if (monster.velocityY < maxFallSpeed)
				monster.velocityY = maxFallSpeed;

			const float deltaY = monster.velocityY * deltaTime;
			const float nextY = monster.y + deltaY;
			if (!IsAABBBlocked(GetMonsterAABB(monster.x, nextY), collisionInset))
			{
				monster.y = nextY;
				monster.onGround = false;
			}
			else if (deltaY < 0.0f)
			{
				const int tileY = WorldToTileY(collib::Bottom(GetMonsterAABB(monster.x, nextY)));
				monster.y = TileTop(tileY) + MonsterCollisionHeight * 0.5f + skin;
				monster.velocityY = 0.0f;
				monster.onGround = true;
			}
			else if (deltaY > 0.0f)
			{
				const int tileY = WorldToTileY(collib::Top(GetMonsterAABB(monster.x, nextY)));
				monster.y = TileBottom(tileY) - MonsterCollisionHeight * 0.5f - skin;
				monster.velocityY = 0.0f;
				monster.onGround = false;
			}
		}

		if (monster.attackCooldown <= 0.0f &&
			m_playerInvulnerableTimer <= 0.0f &&
			collib::Intersects(GetPlayerAABB(m_player.x, m_player.y), GetMonsterAABB(monster.x, monster.y)))
		{
			m_playerHealth -= monster.underground ? 11 : 8;
			m_playerInvulnerableTimer = 0.75f;
			monster.attackCooldown = monster.underground ? 0.85f : 1.05f;
			if (monster.contactDirection == 0)
				monster.contactDirection = monster.facing;
			monster.contactTimer = (std::max)(monster.contactTimer, 0.28f);
			SetStatusText("ZOMBIE HIT", 0.8f);
		}
	}

	RebuildMonsterSpatialGrid();
	bool separatedMonsters = false;
	for (size_t i = 0; i < m_monsters.size(); ++i)
	{
		MonsterState& a = m_monsters[i];
		if (!a.alive)
			continue;

		QueryMonstersInAABB(GetMonsterAABB(a.x, a.y), m_monsterOverlapScratch);
		for (int candidateIndex : m_monsterOverlapScratch)
		{
			if (candidateIndex <= static_cast<int>(i))
				continue;

			MonsterState& b = m_monsters[candidateIndex];
			if (!b.alive)
				continue;

			const collib::AABB aBox = GetMonsterAABB(a.x, a.y);
			const collib::AABB bBox = GetMonsterAABB(b.x, b.y);
			if (!collib::Intersects(aBox, bBox))
				continue;

			const float overlapX = (std::min)(collib::Right(aBox), collib::Right(bBox)) -
				(std::max)(collib::Left(aBox), collib::Left(bBox));
			const float overlapY = (std::min)(collib::Top(aBox), collib::Top(bBox)) -
				(std::max)(collib::Bottom(aBox), collib::Bottom(bBox));
			if (overlapX <= 0.0f || overlapY <= 0.0f || overlapX > overlapY * 1.6f)
				continue;

			const float direction = a.x <= b.x ? -1.0f : 1.0f;
			const float push = overlapX * 0.5f + 0.08f;
			const float nextAX = a.x + direction * push;
			const float nextBX = b.x - direction * push;
			if (!IsAABBBlocked(GetMonsterAABB(nextAX, a.y), collisionInset))
			{
				a.x = nextAX;
				separatedMonsters = true;
			}
			if (!IsAABBBlocked(GetMonsterAABB(nextBX, b.y), collisionInset))
			{
				b.x = nextBX;
				separatedMonsters = true;
			}

			a.facing = direction < 0.0f ? -1 : 1;
			b.facing = -a.facing;
		}
	}

	if (separatedMonsters)
		RebuildMonsterSpatialGrid();
}

void GameEngine::UpdateDroppedItems(float deltaTime)
{
	const float itemSize = m_tileSize * 0.62f;
	const float gravity = -m_tileSize * 30.0f;
	const float maxFallSpeed = -m_tileSize * 18.0f;
	const float magnetRadius = m_tileSize * 5.2f;
	const float pickupRadius = m_tileSize * 0.82f;
	const float maxMagnetSpeed = m_tileSize * 22.0f;
	const float skin = 0.02f;

	for (DroppedItemState& item : m_droppedItems)
	{
		if (!item.alive)
			continue;

		if (item.pickupDelay > 0.0f)
			item.pickupDelay -= deltaTime;

		const float deltaToPlayerX = m_player.x - item.x;
		const float deltaToPlayerY = m_player.y - item.y;
		const float distanceSq = deltaToPlayerX * deltaToPlayerX + deltaToPlayerY * deltaToPlayerY;
		if (item.pickupDelay <= 0.0f && distanceSq <= pickupRadius * pickupRadius)
		{
			AddBlockToInventory(item.tileIndex, item.amount);
			item.alive = false;
			continue;
		}

		if (item.pickupDelay <= 0.0f && distanceSq <= magnetRadius * magnetRadius)
		{
			const float distance = std::sqrt((std::max)(distanceSq, 0.001f));
			const float pullStrength = 1.0f - Clamp01(distance / magnetRadius);
			item.velocityX += (deltaToPlayerX / distance) * m_tileSize * (36.0f + pullStrength * 48.0f) * deltaTime;
			item.velocityY += (deltaToPlayerY / distance) * m_tileSize * (36.0f + pullStrength * 48.0f) * deltaTime;

			const float speedSq = item.velocityX * item.velocityX + item.velocityY * item.velocityY;
			if (speedSq > maxMagnetSpeed * maxMagnetSpeed)
			{
				const float speed = std::sqrt(speedSq);
				item.velocityX = (item.velocityX / speed) * maxMagnetSpeed;
				item.velocityY = (item.velocityY / speed) * maxMagnetSpeed;
			}
		}
		else
		{
			item.velocityY += gravity * deltaTime;
			if (item.velocityY < maxFallSpeed)
				item.velocityY = maxFallSpeed;
			item.velocityX *= (std::max)(0.0f, 1.0f - deltaTime * 2.8f);
		}

		const float nextX = item.x + item.velocityX * deltaTime;
		const collib::AABB horizontalBox = collib::MakeAABB(nextX, item.y, itemSize, itemSize);
		if (!IsAABBBlocked(horizontalBox, 0.0f))
		{
			item.x = nextX;
		}
		else
		{
			item.velocityX *= -0.25f;
		}

		const float deltaY = item.velocityY * deltaTime;
		const float nextY = item.y + deltaY;
		const collib::AABB verticalBox = collib::MakeAABB(item.x, nextY, itemSize, itemSize);
		if (!IsAABBBlocked(verticalBox, 0.0f))
		{
			item.y = nextY;
		}
		else if (deltaY < 0.0f)
		{
			const int tileY = WorldToTileY(collib::Bottom(verticalBox));
			item.y = TileTop(tileY) + itemSize * 0.5f + skin;
			item.velocityY = 0.0f;
			item.velocityX *= 0.72f;
		}
		else if (deltaY > 0.0f)
		{
			const int tileY = WorldToTileY(collib::Top(verticalBox));
			item.y = TileBottom(tileY) - itemSize * 0.5f - skin;
			item.velocityY = 0.0f;
		}
	}
}

void GameEngine::UpdateBlockBreaking(float deltaTime)
{
	int miningBlockIndex = -1;
	if (m_input != nullptr && IsKeyHeld(VK_LBUTTON))
	{
		int tileX = 0;
		int tileY = 0;
		if (GetHoveredBlockTile(tileX, tileY) && IsTileNearPlayer(tileX, tileY, InteractionRangeTiles))
			miningBlockIndex = tileY * m_blockWidth + tileX;
	}

	for (BlockBreakState& breakState : m_blockBreaks)
		breakState.active = 0;

	if (m_uiConsumesLeftMouse)
		return;

	if (miningBlockIndex < 0 || miningBlockIndex >= static_cast<int>(m_blockBreaks.size()))
		return;

	if (m_blocks[miningBlockIndex].visible == 0)
	{
		m_blockBreaks[miningBlockIndex] = BlockBreakState();
		return;
	}

	BlockBreakState& breakState = m_blockBreaks[miningBlockIndex];
	breakState.active = 1;
	breakState.idleTime = 0.0f;
	breakState.progress += deltaTime / GetBlockBreakDuration(m_blocks[miningBlockIndex].tileIndex);
	if (breakState.progress < 1.0f)
		return;

	const int tileX = miningBlockIndex % m_blockWidth;
	const int tileY = miningBlockIndex / m_blockWidth;
	TryHarvestTreeAt(tileX, tileY);
	if (m_blocks[miningBlockIndex].visible != 0)
	{
		const float dropX = m_worldOriginX + tileX * m_tileSize;
		const float dropY = m_worldOriginY - tileY * m_tileSize;
		const unsigned short tileIndex = m_blocks[miningBlockIndex].tileIndex;
		if (tileIndex != BlockLeaves)
			SpawnDroppedItem(dropX, dropY, tileIndex, 1, tileIndex != BlockWood);
		m_blocks[miningBlockIndex].visible = 0;
		MarkBlockChunkDirty(tileX, tileY);
	}

	breakState.active = 0;
	breakState.progress = 0.0f;
	breakState.idleTime = 0.0f;
}

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
		const int biome = m_biomes.empty() ? BiomeGrassland : std::clamp(static_cast<int>(m_biomes[tileX]), 0, BackgroundBiomeCount - 1);
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
	DrawBackground();

	BlockGridDesc gridDesc;
	gridDesc.textureFile = BlockAtlasTexture;
	gridDesc.tiles = m_blocks.data();
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

	m_renderer->DrawBlockGrid(gridDesc);
	DrawBlockCracks();
	DrawHoveredBlockOutline();
	DrawDroppedItems();
	DrawMonsters();

	const bool wantsLeft = IsKeyHeld(KeyA);
	const bool wantsRight = IsKeyHeld(KeyD);
	const bool isMoving = wantsLeft != wantsRight;
	const float playerSpriteYOffset = -m_playerCollisionHeight * 0.5f + m_playerDrawHeight * 0.5f;

	SpriteDesc playerDesc;
	playerDesc.positionX = m_player.x - m_cameraX;
	playerDesc.positionY = m_player.y - m_cameraY + playerSpriteYOffset;
	playerDesc.width = m_playerDrawWidth;
	playerDesc.height = m_playerDrawHeight;
	playerDesc.flipX = m_player.facing > 0 ? 1 : 0;
	playerDesc.depth = -1.0f;
	playerDesc.textureFile = PlayerSpriteTexture;
	playerDesc.atlasColumns = PlayerSpriteColumns;
	playerDesc.atlasRows = PlayerSpriteRows;
	if (m_attackTimer > 0.0f)
	{
		const float attackProgress = 1.0f - Clamp01(m_attackTimer / PlayerAttackDuration);
		const float lunge = std::sin(attackProgress * 3.1415926f);
		playerDesc.positionX += m_player.facing * (3.0f + lunge * 4.5f);
		playerDesc.positionY -= lunge * 1.4f;
		playerDesc.rotationRadians = m_player.facing * (0.035f + lunge * 0.060f);
	}

	if (!m_player.onGround)
	{
		playerDesc.tileIndex = PlayerJumpFrame;
	}
	else if (isMoving)
	{
		const int frame = static_cast<int>(m_player.animationTime / 0.11f) % 4;
		playerDesc.tileIndex = PlayerWalkFrameStart + frame;
	}
	else
	{
		playerDesc.tileIndex = PlayerIdleFrameStart + static_cast<int>(m_player.animationTime / 0.24f) % 4;
	}

	m_renderer->DrawSprite(playerDesc);
	DrawPlayerAttackMotion();
	DrawAttackArc();
	DrawInventory();
	DrawMinimap();
	DrawPlayerStatus();
	DrawCraftingPanel();
	DrawCraftingPrompt();
	DrawFrameStats();
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
			const BlockBreakState& breakState = m_blockBreaks[blockIndex];
			if (!breakState.active || m_blocks[blockIndex].visible == 0)
				continue;

			const float progress = Clamp01(breakState.progress);
			const float centerX = m_worldOriginX + x * m_tileSize - m_cameraX;
			const float centerY = m_worldOriginY - y * m_tileSize - m_cameraY;
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
		const int frame = moving ? static_cast<int>(monster.animationTime) % MonsterSpriteColumns : 0;

		SpriteDesc shadowDesc;
		shadowDesc.textureFile = nullptr;
		shadowDesc.positionX = drawX;
		shadowDesc.positionY = drawY - MonsterCollisionHeight * 0.52f;
		shadowDesc.width = 20.0f;
		shadowDesc.height = 3.2f;
		shadowDesc.colorA = 0.24f;
		shadowDesc.depth = -0.86f;
		m_renderer->DrawSprite(shadowDesc);

		SpriteDesc monsterDesc;
		monsterDesc.textureFile = MonsterSpriteTextures[biome];
		monsterDesc.positionX = drawX + recoilX;
		monsterDesc.positionY = baseY + MonsterSpriteYOffset;
		monsterDesc.width = MonsterSpriteDrawSize;
		monsterDesc.height = MonsterSpriteDrawSize;
		monsterDesc.atlasColumns = MonsterSpriteColumns;
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
		const float barWidth = 18.0f;
		const float barY = baseY + MonsterSpriteYOffset + MonsterSpriteDrawSize * 0.46f;
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
		itemDesc.tileIndex = item.tileIndex;
		itemDesc.depth = -0.90f;
		m_renderer->DrawSprite(itemDesc);
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
	const bool swordEquipped = m_selectedInventorySlot == SlotSword && m_inventoryCounts[SlotSword] > 0;
	const bool axeEquipped = m_selectedInventorySlot == SlotAxe && m_inventoryCounts[SlotAxe] > 0;
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
	const float slotSize = 42.0f;
	const float slotGap = 6.0f;
	const float totalWidth = slotSize * InventorySlotCount + slotGap * (InventorySlotCount - 1);
	const float slotY = -GetViewHalfHeight() + 38.0f;
	const float startX = -totalWidth * 0.5f + slotSize * 0.5f;

	for (int slot = 0; slot < InventorySlotCount; ++slot)
	{
		const float slotX = startX + slot * (slotSize + slotGap);
		const bool selected = slot == m_selectedInventorySlot;

		SpriteDesc backgroundDesc;
		backgroundDesc.textureFile = nullptr;
		backgroundDesc.positionX = slotX;
		backgroundDesc.positionY = slotY;
		backgroundDesc.width = slotSize;
		backgroundDesc.height = slotSize;
		backgroundDesc.colorR = selected ? 0.16f : 0.035f;
		backgroundDesc.colorG = selected ? 0.15f : 0.035f;
		backgroundDesc.colorB = selected ? 0.09f : 0.04f;
		backgroundDesc.colorA = selected ? 0.76f : 0.58f;
		backgroundDesc.depth = -5.0f;
		m_renderer->DrawSprite(backgroundDesc);

		RectOutlineDesc outlineDesc;
		outlineDesc.positionX = slotX;
		outlineDesc.positionY = slotY;
		outlineDesc.width = slotSize;
		outlineDesc.height = slotSize;
		outlineDesc.thickness = selected ? 3.0f : 1.5f;
		outlineDesc.colorR = selected ? 1.0f : 0.55f;
		outlineDesc.colorG = selected ? 0.86f : 0.58f;
		outlineDesc.colorB = selected ? 0.28f : 0.62f;
		outlineDesc.colorA = selected ? 1.0f : 0.78f;
		outlineDesc.depth = -6.0f;
		m_renderer->DrawRectOutline(outlineDesc);

		if (IsInventoryWeaponSlot(slot))
			DrawWeaponIcon(slotX, slotY + 5.0f, 28.0f, -7.0f, m_inventoryCounts[slot] > 0 ? 1.0f : 0.24f, slot);
	}

	for (int slot = 0; slot < InventorySlotCount; ++slot)
	{
		if (!IsInventoryBlockSlot(slot))
			continue;

		const float slotX = startX + slot * (slotSize + slotGap);
		SpriteDesc blockDesc;
		blockDesc.textureFile = BlockAtlasTexture;
		blockDesc.positionX = slotX;
		blockDesc.positionY = slotY + 5.0f;
		blockDesc.width = 24.0f;
		blockDesc.height = 24.0f;
		blockDesc.atlasColumns = BlockAtlasColumns;
		blockDesc.atlasRows = BlockAtlasRows;
		blockDesc.tileIndex = InventoryTileIndices[slot];
		blockDesc.colorA = m_inventoryCounts[slot] > 0 ? 1.0f : 0.28f;
		blockDesc.depth = -7.0f;
		m_renderer->DrawSprite(blockDesc);
	}

	for (int slot = 0; slot < InventorySlotCount; ++slot)
	{
		const float slotX = startX + slot * (slotSize + slotGap);

		char countText[8] = {};
		std::snprintf(countText, sizeof(countText), "%d", m_inventoryCounts[slot]);
		DrawText(slotX - 14.0f, slotY - 11.0f, countText, 2.0f, 0.0f, 0.0f, 0.0f, 0.75f, -7.5f);
		DrawText(slotX - 15.0f, slotY - 10.0f, countText, 2.0f, 0.92f, 0.94f, 0.86f, 1.0f, -8.0f);

		char slotText[2] = { slot < 9 ? static_cast<char>('1' + slot) : '0', '\0' };
		DrawText(slotX - 16.0f, slotY + 18.0f, slotText, 1.7f, 0.82f, 0.84f, 0.82f, 0.95f, -8.0f);
	}
}

void GameEngine::DrawWeaponIcon(float centerX, float centerY, float size, float depth, float alpha, int slot)
{
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

void GameEngine::DrawMinimap()
{
	const bool expanded = m_minimapExpanded;
	const int sampleStep = expanded ? 1 : 3;
	const int centerTileX = std::clamp(WorldToTileX(m_cameraX), 0, m_blockWidth - 1);
	const int centerTileY = std::clamp(WorldToTileY(m_cameraY), 0, m_blockHeight - 1);
	int startTileX = 0;
	int endTileX = m_blockWidth - 1;
	int startTileY = 0;
	int endTileY = m_blockHeight - 1;

	if (!expanded)
	{
		const int halfTilesX = 42;
		const int halfTilesY = 25;
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

	const float mapWidth = sampleWidth * pixelSize;
	const float mapHeight = sampleHeight * pixelSize;
	const float mapLeft = expanded ? -mapWidth * 0.5f : GetViewHalfWidth() - mapWidth - 14.0f;
	const float mapTop = expanded ? mapHeight * 0.5f : GetViewHalfHeight() - 16.0f;

	if (expanded)
	{
		DrawSolidRect(0.0f, 0.0f, GetViewHalfWidth() * 2.0f, GetViewHalfHeight() * 2.0f,
			0.0f, 0.0f, 0.0f, 0.42f, -5.2f);
	}

	DrawSolidRect(mapLeft + mapWidth * 0.5f, mapTop - mapHeight * 0.5f, mapWidth + 8.0f, mapHeight + 8.0f,
		0.015f, 0.018f, 0.022f, expanded ? 0.88f : 0.74f, -6.0f);

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
				const BlockTile& tile = m_blocks[tileY * m_blockWidth + tileX];
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
			-6.5f);
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
		DrawSolidRect(markerX, markerY, markerSize, markerSize, colorR, colorG, colorB, 1.0f, -7.0f);
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

	RectOutlineDesc outlineDesc;
	outlineDesc.positionX = mapLeft + mapWidth * 0.5f;
	outlineDesc.positionY = mapTop - mapHeight * 0.5f;
	outlineDesc.width = mapWidth + 8.0f;
	outlineDesc.height = mapHeight + 8.0f;
	outlineDesc.thickness = 1.5f;
	outlineDesc.colorR = 0.42f;
	outlineDesc.colorG = 0.45f;
	outlineDesc.colorB = 0.48f;
	outlineDesc.colorA = 0.8f;
	outlineDesc.depth = -7.2f;
	m_renderer->DrawRectOutline(outlineDesc);
}

void GameEngine::DrawFrameStats()
{
	char fpsText[16] = {};
	char frameText[16] = {};
	char capText[16] = {};
	std::snprintf(fpsText, sizeof(fpsText), "FPS %03d", static_cast<int>(m_displayFps + 0.5f));
	std::snprintf(frameText, sizeof(frameText), "%.1fMS", m_displayFrameMs);
	std::snprintf(capText, sizeof(capText), m_frameLimitEnabled ? "CAP %03d" : "CAP OFF", m_targetRefreshRate);

	const float x = -GetViewHalfWidth() + 14.0f;
	const float y = GetViewHalfHeight() - 14.0f;
	DrawText(x + 1.5f, y - 1.5f, fpsText, 2.4f, 0.0f, 0.0f, 0.0f, 0.72f, -7.5f);
	DrawText(x, y, fpsText, 2.4f, 0.66f, 0.95f, 0.72f, 1.0f, -8.0f);
	DrawText(x + 1.5f, y - 22.5f, frameText, 2.0f, 0.0f, 0.0f, 0.0f, 0.72f, -7.5f);
	DrawText(x, y - 21.0f, frameText, 2.0f, 0.86f, 0.88f, 0.82f, 1.0f, -8.0f);
	DrawText(x + 82.2f, y - 22.2f, capText, 1.7f, 0.0f, 0.0f, 0.0f, 0.70f, -7.5f);
	DrawText(x + 81.0f, y - 21.0f, capText, 1.7f, 0.72f, 0.82f, 0.92f, 1.0f, -8.0f);

	if (m_showRenderStats)
		DrawRenderStatsOverlay();
}

void GameEngine::DrawRenderStatsOverlay()
{
	if (m_renderer == nullptr)
		return;

	RenderFrameStats stats;
	m_renderer->GetLastFrameStats(stats);

	char lines[10][36] = {};
	std::snprintf(lines[0], sizeof(lines[0]), "F1 PERF STATS");
	std::snprintf(lines[1], sizeof(lines[1]), "CPU:%.2f DRAW:%.2f", m_displayCpuStats.totalMs, m_displayCpuStats.drawWorldMs);
	std::snprintf(lines[2], sizeof(lines[2]), "SIM:%.2f MON:%.2f", m_displayCpuStats.simulationMs, m_displayCpuStats.monstersMs);
	std::snprintf(lines[3], sizeof(lines[3]), "ITEM:%.2f REN:%.2f", m_displayCpuStats.itemsMs, m_displayCpuStats.renderMs);
	std::snprintf(lines[4], sizeof(lines[4]), "DRAW:%u TEX:%u", stats.drawCalls, stats.textureBinds);
	std::snprintf(lines[5], sizeof(lines[5]), "SPRITE:%u RECT:%u", stats.spriteDrawCalls, stats.rectOutlineCalls);
	std::snprintf(lines[6], sizeof(lines[6]), "GRID:%u INST:%u", stats.gridDrawCalls, stats.gridInstances);
	std::snprintf(lines[7], sizeof(lines[7]), "CH:%u RB:%u", stats.gridChunksDrawn, stats.gridChunksRebuilt);
	std::snprintf(lines[8], sizeof(lines[8]), "QUADS:%u", stats.spriteQuads);
	std::snprintf(lines[9], sizeof(lines[9]), "IN:%.2f", m_displayCpuStats.inputMs);

	const float x = -GetViewHalfWidth() + 14.0f;
	const float y = GetViewHalfHeight() - 112.0f;
	const float lineHeight = 15.0f;
	const float panelWidth = 226.0f;
	const float panelHeight = 153.0f;
	DrawSolidRect(x + panelWidth * 0.5f - 4.0f, y - panelHeight * 0.5f + 8.0f, panelWidth, panelHeight,
		0.015f, 0.018f, 0.022f, 0.70f, -7.1f);

	for (int i = 0; i < 10; ++i)
	{
		const float lineY = y - i * lineHeight;
		DrawText(x + 1.2f, lineY - 1.2f, lines[i], 1.8f, 0.0f, 0.0f, 0.0f, 0.72f, -7.6f);
		DrawText(x, lineY, lines[i], 1.8f, 0.70f, 0.92f, 1.0f, 1.0f, -8.0f);
	}
}

void GameEngine::DrawPlayerStatus()
{
	const float x = -GetViewHalfWidth() + 14.0f;
	const float y = GetViewHalfHeight() - 58.0f;
	const float healthRatio = Clamp01(static_cast<float>(m_playerHealth) / 100.0f);

	DrawText(x + 1.0f, y - 1.0f, "HP", 2.2f, 0.0f, 0.0f, 0.0f, 0.70f, -7.4f);
	DrawText(x, y, "HP", 2.2f, 0.92f, 0.86f, 0.78f, 1.0f, -8.0f);
	DrawSolidRect(x + 64.0f, y - 7.0f, 82.0f, 10.0f, 0.035f, 0.025f, 0.025f, 0.75f, -7.0f);
	DrawSolidRect(x + 23.0f + 82.0f * healthRatio * 0.5f, y - 7.0f,
		82.0f * healthRatio, 8.0f, 0.82f, 0.12f, 0.12f, 1.0f, -8.0f);
}

void GameEngine::DrawCraftingPanel()
{
	if (!IsCraftingTableNearby())
		return;

	const CraftingPanelLayout layout = GetCraftingPanelLayout();
	const float panelCenterX = layout.left + layout.width * 0.5f;
	const float panelCenterY = layout.top - layout.height * 0.5f;

	DrawSolidRect(panelCenterX + 3.0f, panelCenterY - 3.0f, layout.width, layout.height,
		0.0f, 0.0f, 0.0f, 0.34f, -6.2f);
	DrawSolidRect(panelCenterX, panelCenterY, layout.width, layout.height,
		0.030f, 0.034f, 0.038f, 0.86f, -6.5f);

	RectOutlineDesc panelOutline;
	panelOutline.positionX = panelCenterX;
	panelOutline.positionY = panelCenterY;
	panelOutline.width = layout.width;
	panelOutline.height = layout.height;
	panelOutline.thickness = 1.5f;
	panelOutline.colorR = 0.70f;
	panelOutline.colorG = 0.58f;
	panelOutline.colorB = 0.34f;
	panelOutline.colorA = 0.88f;
	panelOutline.depth = -7.2f;
	m_renderer->DrawRectOutline(panelOutline);

	auto drawPanelText = [this](float x, float y, const char* text, float size,
		float colorR, float colorG, float colorB, float depth)
	{
		DrawText(x + 1.1f, y - 1.1f, text, size, 0.0f, 0.0f, 0.0f, 0.72f, depth + 0.35f);
		DrawText(x, y, text, size, colorR, colorG, colorB, 1.0f, depth);
	};

	drawPanelText(layout.left + 13.0f, layout.top - 15.0f, "WORKBENCH", 2.1f, 0.94f, 0.82f, 0.54f, -8.0f);

	float cursorX = 0.0f;
	float cursorY = 0.0f;
	const int hoveredRecipe = GetCursorViewPosition(cursorX, cursorY) ? GetCraftingRecipeAt(cursorX, cursorY) : -1;

	struct CraftingRow
	{
		const char* name;
		int slot;
		int woodCost;
		int stoneCost;
		bool owned;
		bool craftable;
	};

	const CraftingRow rows[2] =
	{
		{ "SWORD", SlotSword, 2, 3, m_inventoryCounts[SlotSword] > 0, CanCraftSword() },
		{ "AXE", SlotAxe, 3, 2, m_inventoryCounts[SlotAxe] > 0, CanCraftAxe() },
	};

	for (int i = 0; i < 2; ++i)
	{
		const CraftingRow& row = rows[i];
		const float rowCenterX = layout.left + layout.width * 0.5f;
		const float rowCenterY = layout.firstRowCenterY - i * (layout.rowHeight + 7.0f);
		const bool active = row.owned || row.craftable;
		const bool hovered = hoveredRecipe == i;

		DrawSolidRect(rowCenterX, rowCenterY, layout.width - 18.0f, layout.rowHeight,
			hovered ? 0.105f : 0.060f,
			active ? 0.090f : 0.060f,
			active ? 0.058f : 0.062f,
			hovered ? 0.96f : 0.82f,
			-6.8f);

		RectOutlineDesc rowOutline;
		rowOutline.positionX = rowCenterX;
		rowOutline.positionY = rowCenterY;
		rowOutline.width = layout.width - 18.0f;
		rowOutline.height = layout.rowHeight;
		rowOutline.thickness = hovered ? 2.0f : 1.0f;
		rowOutline.colorR = active ? 0.74f : 0.38f;
		rowOutline.colorG = active ? 0.66f : 0.38f;
		rowOutline.colorB = active ? 0.42f : 0.40f;
		rowOutline.colorA = hovered ? 1.0f : 0.72f;
		rowOutline.depth = -7.3f;
		m_renderer->DrawRectOutline(rowOutline);

		DrawWeaponIcon(layout.left + 31.0f, rowCenterY + 1.0f, 28.0f, -7.8f, active ? 1.0f : 0.32f, row.slot);
		drawPanelText(layout.left + 57.0f, rowCenterY + 14.0f, row.name, 1.9f,
			active ? 0.92f : 0.58f, active ? 0.88f : 0.58f, active ? 0.76f : 0.60f, -8.0f);

		char costText[32] = {};
		std::snprintf(costText, sizeof(costText), "%d WOOD %d STONE", row.woodCost, row.stoneCost);
		drawPanelText(layout.left + 57.0f, rowCenterY - 6.0f, costText, 1.45f,
			row.craftable || row.owned ? 0.70f : 0.58f,
			row.craftable || row.owned ? 0.84f : 0.56f,
			row.craftable || row.owned ? 0.66f : 0.54f,
			-8.0f);

		const char* stateText = row.owned ? "OWNED" : (row.craftable ? "CRAFT" : "NEED");
		const float badgeX = layout.left + layout.width - 39.0f;
		DrawSolidRect(badgeX, rowCenterY, 54.0f, 20.0f,
			row.owned ? 0.06f : (row.craftable ? 0.10f : 0.05f),
			row.owned ? 0.09f : (row.craftable ? 0.16f : 0.05f),
			row.owned ? 0.12f : (row.craftable ? 0.08f : 0.06f),
			0.90f,
			-7.5f);
		drawPanelText(badgeX - 21.0f, rowCenterY + 5.0f, stateText, 1.35f,
			row.owned ? 0.72f : (row.craftable ? 0.74f : 0.70f),
			row.owned ? 0.88f : (row.craftable ? 0.95f : 0.62f),
			row.owned ? 0.96f : (row.craftable ? 0.70f : 0.58f),
			-8.0f);
	}
}

void GameEngine::DrawCraftingPrompt()
{
	const float x = -GetViewHalfWidth() + 14.0f;
	const float y = GetViewHalfHeight() - 84.0f;
	const char* text = m_statusTextTimer > 0.0f ? m_statusText.data() : nullptr;

	auto drawLine = [this, x](float lineY, const char* lineText, float colorR, float colorG, float colorB)
	{
		DrawText(x + 1.2f, lineY - 1.2f, lineText, 1.9f, 0.0f, 0.0f, 0.0f, 0.68f, -7.4f);
		DrawText(x, lineY, lineText, 1.9f, colorR, colorG, colorB, 1.0f, -8.0f);
	};

	if (text != nullptr && text[0] != '\0')
	{
		drawLine(y, text, 0.86f, 0.88f, 0.80f);
		return;
	}

	if (IsCraftingTableNearby())
		return;

	if (m_inventoryCounts[SlotCraftingTable] > 0)
		drawLine(y, "PLACE TABLE SLOT 8", 0.86f, 0.88f, 0.80f);
	else
		drawLine(y, "C MAKE TABLE", 0.86f, 0.88f, 0.80f);
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

	float cursorX = x;
	for (const char* cursor = text; *cursor != '\0'; ++cursor)
	{
		if (*cursor == ' ')
		{
			cursorX += pixelSize * 4.0f;
			continue;
		}

		DrawGlyph(cursorX, y, *cursor, pixelSize, colorR, colorG, colorB, colorA, depth);
		cursorX += pixelSize * 6.0f;
	}
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
	layout.left = -GetViewHalfWidth() + 14.0f;
	layout.top = GetViewHalfHeight() - 112.0f;
	layout.width = 226.0f;
	layout.height = 158.0f;
	layout.rowHeight = 48.0f;
	layout.firstRowCenterY = layout.top - 62.0f;
	return layout;
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

int GameEngine::GetCraftingRecipeAt(float viewX, float viewY) const
{
	if (!IsCraftingTableNearby())
		return -1;

	const CraftingPanelLayout layout = GetCraftingPanelLayout();
	if (viewX < layout.left || viewX > layout.left + layout.width ||
		viewY > layout.top || viewY < layout.top - layout.height)
	{
		return -1;
	}

	for (int recipe = 0; recipe < 2; ++recipe)
	{
		const float rowCenterY = layout.firstRowCenterY - recipe * (layout.rowHeight + 7.0f);
		if (viewY <= rowCenterY + layout.rowHeight * 0.5f &&
			viewY >= rowCenterY - layout.rowHeight * 0.5f)
		{
			return recipe;
		}
	}

	return -1;
}

bool GameEngine::IsCursorOverCraftingPanel() const
{
	float cursorX = 0.0f;
	float cursorY = 0.0f;
	if (!GetCursorViewPosition(cursorX, cursorY) || !IsCraftingTableNearby())
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
	const bool occupied = m_blocks[blockIndex].visible != 0;
	const bool canPlace = CanPlaceSelectedBlockAt(tileX, tileY);

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

bool GameEngine::GetHoveredTile(int& tileX, int& tileY) const
{
	float viewX = 0.0f;
	float viewY = 0.0f;
	if (!GetCursorViewPosition(viewX, viewY))
		return false;

	const float worldX = viewX + m_cameraX;
	const float worldY = viewY + m_cameraY;

	tileX = WorldToTileX(worldX);
	tileY = WorldToTileY(worldY);
	if (tileX < 0 || tileX >= m_blockWidth || tileY < 0 || tileY >= m_blockHeight)
		return false;

	return true;
}

bool GameEngine::GetHoveredBlockTile(int& tileX, int& tileY) const
{
	if (!GetHoveredTile(tileX, tileY))
		return false;

	return m_blocks[tileY * m_blockWidth + tileX].visible != 0;
}

collib::AABB GameEngine::GetPlayerAABB(float playerX, float playerY) const
{
	return collib::MakeAABB(playerX, playerY, m_playerCollisionWidth, m_playerCollisionHeight);
}

collib::AABB GameEngine::GetMonsterAABB(float monsterX, float monsterY) const
{
	return collib::MakeAABB(monsterX, monsterY, MonsterCollisionWidth, MonsterCollisionHeight);
}

collib::AABB GameEngine::GetTileAABB(int tileX, int tileY) const
{
	return collib::MakeAABB(
		m_worldOriginX + tileX * m_tileSize,
		m_worldOriginY - tileY * m_tileSize,
		m_tileSize,
		m_tileSize);
}

bool GameEngine::IsKeyHeld(int keyCode)
{
	return m_input->IsDown(keyCode, this) || m_input->IsPressed(keyCode, this);
}

bool GameEngine::IsTileInBounds(int tileX, int tileY) const
{
	return tileX >= 0 && tileX < m_blockWidth && tileY >= 0 && tileY < m_blockHeight;
}

bool GameEngine::IsSolidTile(int tileX, int tileY) const
{
	if (tileY < 0)
		return false;
	if (tileX < 0 || tileX >= m_blockWidth || tileY >= m_blockHeight)
		return true;

	const BlockTile& tile = m_blocks[tileY * m_blockWidth + tileX];
	if (tile.visible == 0)
		return false;

	return tile.tileIndex != BlockWood && tile.tileIndex != BlockLeaves;
}

bool GameEngine::IsTileNearPlayer(int tileX, int tileY, float maxTiles) const
{
	const float centerX = m_worldOriginX + tileX * m_tileSize;
	const float centerY = m_worldOriginY - tileY * m_tileSize;
	const float deltaX = centerX - m_player.x;
	const float deltaY = centerY - m_player.y;
	const float maxDistance = maxTiles * m_tileSize;
	return deltaX * deltaX + deltaY * deltaY <= maxDistance * maxDistance;
}

bool GameEngine::IsInventoryBlockSlot(int slot) const
{
	return slot >= 0 && slot < BlockInventorySlotCount;
}

bool GameEngine::IsInventoryWeaponSlot(int slot) const
{
	return slot == SlotSword || slot == SlotAxe;
}

bool GameEngine::IsMapTileRevealed(int tileX, int tileY) const
{
	if (!IsTileInBounds(tileX, tileY))
		return false;
	if (m_debugRevealMap)
		return true;
	if (m_revealedTiles.size() != m_blocks.size())
		return false;

	return m_revealedTiles[tileY * m_blockWidth + tileX] != 0;
}

bool GameEngine::CanCraftSword() const
{
	return m_inventoryCounts[SlotWood] >= 2 && m_inventoryCounts[SlotStone] >= 3;
}

bool GameEngine::CanCraftAxe() const
{
	return m_inventoryCounts[SlotWood] >= 3 && m_inventoryCounts[SlotStone] >= 2;
}

bool GameEngine::CanPlaceBlockAt(int tileX, int tileY) const
{
	if (!IsTileInBounds(tileX, tileY))
		return false;

	const int blockIndex = tileY * m_blockWidth + tileX;
	if (m_blocks[blockIndex].visible != 0)
		return false;

	if (!IsTileNearPlayer(tileX, tileY, InteractionRangeTiles))
		return false;

	if (collib::Intersects(GetPlayerAABB(m_player.x, m_player.y), GetTileAABB(tileX, tileY)))
		return false;

	const bool hasNeighbor =
		(tileX > 0 && IsSolidTile(tileX - 1, tileY)) ||
		(tileX < m_blockWidth - 1 && IsSolidTile(tileX + 1, tileY)) ||
		(tileY > 0 && IsSolidTile(tileX, tileY - 1)) ||
		(tileY < m_blockHeight - 1 && IsSolidTile(tileX, tileY + 1));

	return hasNeighbor;
}

bool GameEngine::CanPlaceSelectedBlockAt(int tileX, int tileY) const
{
	if (!IsInventoryBlockSlot(m_selectedInventorySlot))
		return false;

	if (m_inventoryCounts[m_selectedInventorySlot] <= 0)
		return false;

	return CanPlaceBlockAt(tileX, tileY);
}

bool GameEngine::ShouldLeftClickAttack() const
{
	int tileX = 0;
	int tileY = 0;
	if (!GetHoveredTile(tileX, tileY))
		return true;

	const int blockIndex = tileY * m_blockWidth + tileX;
	return m_blocks[blockIndex].visible == 0;
}

bool GameEngine::IsCraftingTableNearby() const
{
	const int playerTileX = WorldToTileX(m_player.x);
	const int playerTileY = WorldToTileY(m_player.y);
	const int searchRadius = 4;

	for (int tileY = playerTileY - searchRadius; tileY <= playerTileY + searchRadius; ++tileY)
	{
		for (int tileX = playerTileX - searchRadius; tileX <= playerTileX + searchRadius; ++tileX)
		{
			if (!IsTileInBounds(tileX, tileY))
				continue;

			const BlockTile& tile = m_blocks[tileY * m_blockWidth + tileX];
			if (tile.visible != 0 && tile.tileIndex == BlockCraftingTable && IsTileNearPlayer(tileX, tileY, 4.5f))
				return true;
		}
	}

	return false;
}

bool GameEngine::TryHarvestTreeAt(int tileX, int tileY)
{
	if (!IsTileInBounds(tileX, tileY))
		return false;

	const int startX = tileX;
	const int startY = tileY;
	const int blockIndex = tileY * m_blockWidth + tileX;
	if (m_blocks[blockIndex].visible == 0 || m_blocks[blockIndex].tileIndex != BlockWood)
		return false;

	std::vector<unsigned char> visited(m_blocks.size(), 0);
	std::vector<int> queue;
	std::vector<int> woodIndices;
	queue.reserve(64);
	woodIndices.reserve(48);

	auto tryAddWood = [&](int nextX, int nextY)
	{
		if (!IsTileInBounds(nextX, nextY))
			return;
		if (nextY > startY)
			return;

		const int nextIndex = nextY * m_blockWidth + nextX;
		if (visited[nextIndex] != 0)
			return;

		const BlockTile& tile = m_blocks[nextIndex];
		if (tile.visible == 0 || tile.tileIndex != BlockWood)
			return;

		visited[nextIndex] = 1;
		queue.push_back(nextIndex);
	};

	tryAddWood(startX, startY);
	for (size_t head = 0; head < queue.size(); ++head)
	{
		const int currentIndex = queue[head];
		woodIndices.push_back(currentIndex);
		const int currentX = currentIndex % m_blockWidth;
		const int currentY = currentIndex / m_blockWidth;
		tryAddWood(currentX - 1, currentY);
		tryAddWood(currentX + 1, currentY);
		tryAddWood(currentX, currentY - 1);
		tryAddWood(currentX, currentY + 1);
	}

	std::vector<int> fallingWoodIndices;
	fallingWoodIndices.reserve(woodIndices.size());
	for (int woodIndex : woodIndices)
	{
		if (woodIndex != blockIndex)
			fallingWoodIndices.push_back(woodIndex);
	}

	std::vector<int> leafIndices;
	leafIndices.reserve(96);
	constexpr int LeafSearchRadius = 5;

	auto nearestHarvestedWoodDistance = [&](int leafX, int leafY)
	{
		int bestDistance = 9999;
		for (int woodIndex : woodIndices)
		{
			const int woodX = woodIndex % m_blockWidth;
			const int woodY = woodIndex / m_blockWidth;
			const int distance = std::abs(leafX - woodX) + std::abs(leafY - woodY);
			if (distance < bestDistance)
				bestDistance = distance;
		}

		return bestDistance;
	};

	auto isClaimedByRemainingTree = [&](int leafX, int leafY, int harvestedDistance)
	{
		for (int offsetY = -LeafSearchRadius; offsetY <= LeafSearchRadius; ++offsetY)
		{
			for (int offsetX = -LeafSearchRadius; offsetX <= LeafSearchRadius; ++offsetX)
			{
				const int woodX = leafX + offsetX;
				const int woodY = leafY + offsetY;
				if (!IsTileInBounds(woodX, woodY))
					continue;

				const int woodIndex = woodY * m_blockWidth + woodX;
				const BlockTile& tile = m_blocks[woodIndex];
				if (tile.visible == 0 || tile.tileIndex != BlockWood || visited[woodIndex] == 1)
					continue;

				const int distance = std::abs(offsetX) + std::abs(offsetY);
				if (distance <= harvestedDistance + 1)
					return true;
			}
		}

		return false;
	};

	auto tryAddLeaf = [&](int nextX, int nextY)
	{
		if (!IsTileInBounds(nextX, nextY))
			return;
		if (nextY > startY + 2)
			return;

		const int nextIndex = nextY * m_blockWidth + nextX;
		if (visited[nextIndex] != 0)
			return;

		const BlockTile& tile = m_blocks[nextIndex];
		if (tile.visible == 0 || tile.tileIndex != BlockLeaves)
			return;

		const int harvestedDistance = nearestHarvestedWoodDistance(nextX, nextY);
		if (harvestedDistance > LeafSearchRadius + 2)
			return;
		if (isClaimedByRemainingTree(nextX, nextY, harvestedDistance))
			return;

		visited[nextIndex] = 2;
		leafIndices.push_back(nextIndex);
	};

	for (int woodIndex : woodIndices)
	{
		const int currentX = woodIndex % m_blockWidth;
		const int currentY = woodIndex / m_blockWidth;
		for (int offsetY = -LeafSearchRadius; offsetY <= LeafSearchRadius; ++offsetY)
		{
			for (int offsetX = -LeafSearchRadius; offsetX <= LeafSearchRadius; ++offsetX)
			{
				if (std::abs(offsetX) + std::abs(offsetY) > LeafSearchRadius + 2)
					continue;

				tryAddLeaf(currentX + offsetX, currentY + offsetY);
			}
		}
	}

	if (fallingWoodIndices.empty() && leafIndices.empty())
		return false;

	for (int woodIndex : fallingWoodIndices)
	{
		m_blocks[woodIndex].visible = 0;
		if (woodIndex >= 0 && woodIndex < static_cast<int>(m_blockBreaks.size()))
			m_blockBreaks[woodIndex] = BlockBreakState();
		MarkBlockIndexDirty(woodIndex);
	}

	for (int leafIndex : leafIndices)
	{
		m_blocks[leafIndex].visible = 0;
		if (leafIndex >= 0 && leafIndex < static_cast<int>(m_blockBreaks.size()))
			m_blockBreaks[leafIndex] = BlockBreakState();
		MarkBlockIndexDirty(leafIndex);
	}

	m_minimapDirty = true;
	for (int woodIndex : fallingWoodIndices)
	{
		const int woodX = woodIndex % m_blockWidth;
		const int woodY = woodIndex / m_blockWidth;
		const float dropX = m_worldOriginX + woodX * m_tileSize;
		const float dropY = m_worldOriginY - woodY * m_tileSize;
		SpawnDroppedItem(dropX, dropY, BlockWood, 1, false);
	}

	return true;
}

float GameEngine::GetBlockBreakDuration(unsigned short tileIndex) const
{
	const bool axeEquipped = m_selectedInventorySlot == SlotAxe && m_inventoryCounts[SlotAxe] > 0;
	if (axeEquipped && (tileIndex == BlockWood || tileIndex == BlockLeaves))
		return BlockBreakDuration * 0.22f;

	return BlockBreakDuration;
}

bool GameEngine::IsGroundBelowBox(float centerX, float centerY, float width, float height) const
{
	const float inset = 1.0f;
	const float probeDistance = 1.0f;
	const float probeWidth = width - inset * 2.0f;
	const float probeY = centerY - height * 0.5f - probeDistance * 0.5f;
	const collib::AABB groundProbe = collib::MakeAABB(centerX, probeY, probeWidth, probeDistance);
	return IsAABBBlocked(groundProbe, 0.0f);
}

void GameEngine::SetStatusText(const char* text, float duration)
{
	if (text == nullptr)
	{
		m_statusText[0] = '\0';
		m_statusTextTimer = 0.0f;
		return;
	}

	std::snprintf(m_statusText.data(), m_statusText.size(), "%s", text);
	m_statusTextTimer = duration;
}

void GameEngine::AddBlockToInventory(unsigned short tileIndex, int amount)
{
	if (amount <= 0)
		return;

	switch (tileIndex)
	{
	case BlockPrairieStone:
	case BlockMossStone:
	case BlockSandstone:
	case BlockDesertStone:
	case BlockFrozenStone:
	case BlockIce:
		tileIndex = BlockStone;
		break;
	case BlockCrystalOre:
		tileIndex = BlockOre;
		break;
	case BlockSnow:
		tileIndex = BlockSand;
		break;
	default:
		break;
	}

	for (int slot = 0; slot < BlockInventorySlotCount; ++slot)
	{
		if (InventoryTileIndices[slot] == tileIndex)
		{
			m_inventoryCounts[slot] += amount;
			return;
		}
	}
}

void GameEngine::SpawnDroppedItem(float worldX, float worldY, unsigned short tileIndex, int amount, bool mergeNearby)
{
	if (amount <= 0)
		return;

	const float mergeDistance = m_tileSize * 1.35f;
	if (mergeNearby)
	{
		for (DroppedItemState& item : m_droppedItems)
		{
			if (!item.alive || item.tileIndex != tileIndex)
				continue;

			const float dx = item.x - worldX;
			const float dy = item.y - worldY;
			if (dx * dx + dy * dy > mergeDistance * mergeDistance)
				continue;

			item.amount += amount;
			item.pickupDelay = (std::max)(item.pickupDelay, 0.18f);
			item.velocityY = (std::max)(item.velocityY, m_tileSize * 4.0f);
			return;
		}
	}

	DroppedItemState item;
	item.x = worldX;
	item.y = worldY;
	item.tileIndex = tileIndex;
	item.amount = amount;
	item.alive = true;
	item.pickupDelay = 0.24f;

	const int hashX = WorldToTileX(worldX);
	const int hashY = WorldToTileY(worldY);
	const float scatter = Hash01(hashX, hashY, GetTickCount()) - 0.5f;
	const float pop = Hash01(hashX + 13, hashY - 7, GetTickCount() + 91u);
	item.velocityX = scatter * m_tileSize * 5.0f;
	item.velocityY = m_tileSize * (4.4f + pop * 2.8f);

	for (DroppedItemState& existing : m_droppedItems)
	{
		if (existing.alive)
			continue;

		existing = item;
		return;
	}

	m_droppedItems.push_back(item);
}

void GameEngine::InitializeBlockChunkCache()
{
	const int chunkSize = (std::max)(1, m_blockChunkSizeTiles);
	m_blockChunkSizeTiles = chunkSize;
	m_blockChunkColumns = (std::max)(1, (m_blockWidth + chunkSize - 1) / chunkSize);
	m_blockChunkRows = (std::max)(1, (m_blockHeight + chunkSize - 1) / chunkSize);

	++m_blockGridVersion;
	if (m_blockGridVersion == 0)
		m_blockGridVersion = 1;

	++m_blockChunkVersionCounter;
	if (m_blockChunkVersionCounter == 0)
		m_blockChunkVersionCounter = 1;

	m_blockChunkVersions.assign(
		static_cast<size_t>(m_blockChunkColumns * m_blockChunkRows),
		m_blockChunkVersionCounter);
}

void GameEngine::MarkBlockChunkDirty(int tileX, int tileY)
{
	if (m_blockChunkVersions.empty())
		return;

	if (!IsTileInBounds(tileX, tileY))
		return;

	const int chunkX = std::clamp(tileX / (std::max)(1, m_blockChunkSizeTiles), 0, m_blockChunkColumns - 1);
	const int chunkY = std::clamp(tileY / (std::max)(1, m_blockChunkSizeTiles), 0, m_blockChunkRows - 1);
	const int chunkIndex = chunkY * m_blockChunkColumns + chunkX;
	if (chunkIndex < 0 || chunkIndex >= static_cast<int>(m_blockChunkVersions.size()))
		return;

	++m_blockChunkVersionCounter;
	if (m_blockChunkVersionCounter == 0)
	{
		m_blockChunkVersionCounter = 1;
		++m_blockGridVersion;
		if (m_blockGridVersion == 0)
			m_blockGridVersion = 1;
		std::fill(m_blockChunkVersions.begin(), m_blockChunkVersions.end(), m_blockChunkVersionCounter);
	}

	m_blockChunkVersions[chunkIndex] = m_blockChunkVersionCounter;
	m_minimapDirty = true;
}

void GameEngine::MarkBlockIndexDirty(int blockIndex)
{
	if (blockIndex < 0 || blockIndex >= static_cast<int>(m_blocks.size()))
		return;

	MarkBlockChunkDirty(blockIndex % m_blockWidth, blockIndex / m_blockWidth);
}

void GameEngine::RebuildMonsterSpatialGrid()
{
	const int cellTiles = (std::max)(1, m_monsterGridCellTiles);
	m_monsterGridColumns = (std::max)(1, (m_blockWidth + cellTiles - 1) / cellTiles);
	m_monsterGridRows = (std::max)(1, (m_blockHeight + cellTiles - 1) / cellTiles);
	const size_t gridCellCount = static_cast<size_t>(m_monsterGridColumns * m_monsterGridRows);
	if (m_monsterGridHeads.size() != gridCellCount)
		m_monsterGridHeads.assign(gridCellCount, -1);
	else
		std::fill(m_monsterGridHeads.begin(), m_monsterGridHeads.end(), -1);

	if (m_monsterGridNext.size() != m_monsters.size())
		m_monsterGridNext.assign(m_monsters.size(), -1);
	else
		std::fill(m_monsterGridNext.begin(), m_monsterGridNext.end(), -1);

	for (size_t i = 0; i < m_monsters.size(); ++i)
	{
		const MonsterState& monster = m_monsters[i];
		if (!monster.alive)
			continue;

		const int tileX = std::clamp(WorldToTileX(monster.x), 0, m_blockWidth - 1);
		const int tileY = std::clamp(WorldToTileY(monster.y), 0, m_blockHeight - 1);
		const int cellX = std::clamp(tileX / cellTiles, 0, m_monsterGridColumns - 1);
		const int cellY = std::clamp(tileY / cellTiles, 0, m_monsterGridRows - 1);
		const int cellIndex = cellY * m_monsterGridColumns + cellX;
		m_monsterGridNext[i] = m_monsterGridHeads[cellIndex];
		m_monsterGridHeads[cellIndex] = static_cast<int>(i);
	}
}

void GameEngine::QueryMonstersInAABB(const collib::AABB& area, std::vector<int>& results) const
{
	results.clear();
	if (m_monsterGridHeads.empty() || m_monsterGridNext.size() != m_monsters.size() ||
		m_monsterGridColumns <= 0 || m_monsterGridRows <= 0)
	{
		for (size_t i = 0; i < m_monsters.size(); ++i)
		{
			if (m_monsters[i].alive && collib::Intersects(area, GetMonsterAABB(m_monsters[i].x, m_monsters[i].y)))
				results.push_back(static_cast<int>(i));
		}
		return;
	}

	int startX = WorldToTileX(collib::Left(area));
	int endX = WorldToTileX(collib::Right(area));
	int startY = WorldToTileY(collib::Top(area));
	int endY = WorldToTileY(collib::Bottom(area));
	if (startX > endX)
	{
		const int temp = startX;
		startX = endX;
		endX = temp;
	}
	if (startY > endY)
	{
		const int temp = startY;
		startY = endY;
		endY = temp;
	}

	const int cellTiles = (std::max)(1, m_monsterGridCellTiles);
	const int minCellX = std::clamp(startX / cellTiles, 0, m_monsterGridColumns - 1);
	const int maxCellX = std::clamp(endX / cellTiles, 0, m_monsterGridColumns - 1);
	const int minCellY = std::clamp(startY / cellTiles, 0, m_monsterGridRows - 1);
	const int maxCellY = std::clamp(endY / cellTiles, 0, m_monsterGridRows - 1);

	for (int cellY = minCellY; cellY <= maxCellY; ++cellY)
	{
		for (int cellX = minCellX; cellX <= maxCellX; ++cellX)
		{
			const int cellIndex = cellY * m_monsterGridColumns + cellX;
			for (int monsterIndex = m_monsterGridHeads[cellIndex]; monsterIndex >= 0; monsterIndex = m_monsterGridNext[monsterIndex])
			{
				if (monsterIndex >= static_cast<int>(m_monsters.size()))
					continue;

				const MonsterState& monster = m_monsters[monsterIndex];
				if (!monster.alive || !collib::Intersects(area, GetMonsterAABB(monster.x, monster.y)))
					continue;

				results.push_back(monsterIndex);
			}
		}
	}
}

float GameEngine::GetViewHalfWidth() const
{
	if (m_window == nullptr || m_window->GetHeight() <= 0.0f)
		return GetViewHalfHeight();

	return GetViewHalfHeight() * (m_window->GetWidth() / m_window->GetHeight());
}

float GameEngine::GetViewHalfHeight() const
{
	constexpr float renderCameraDistance = 500.0f;
	constexpr float renderFovYDegrees = 60.0f;
	return std::tan((renderFovYDegrees * Deg2Rad) * 0.5f) * renderCameraDistance;
}

float GameEngine::GetSurfaceWorldYAt(float worldX) const
{
	if (m_surfaceHeights.empty())
		return TileTop(m_surfaceRow);

	int tileX = WorldToTileX(worldX);
	if (tileX < 0)
		tileX = 0;
	else if (tileX >= static_cast<int>(m_surfaceHeights.size()))
		tileX = static_cast<int>(m_surfaceHeights.size()) - 1;

	return TileTop(m_surfaceHeights[tileX]);
}

bool GameEngine::IsAABBBlocked(const collib::AABB& box, float inset) const
{
	const collib::AABB testBox = collib::Inset(box, inset, inset);
	int startX = WorldToTileX(collib::Left(testBox));
	int endX = WorldToTileX(collib::Right(testBox));
	int startY = WorldToTileY(collib::Top(testBox));
	int endY = WorldToTileY(collib::Bottom(testBox));
	if (startX > endX)
	{
		const int temp = startX;
		startX = endX;
		endX = temp;
	}
	if (startY > endY)
	{
		const int temp = startY;
		startY = endY;
		endY = temp;
	}

	for (int y = startY; y <= endY; ++y)
	{
		for (int x = startX; x <= endX; ++x)
		{
			if (!IsSolidTile(x, y))
				continue;

			if (collib::Intersects(testBox, GetTileAABB(x, y)))
				return true;
		}
	}

	return false;
}

bool GameEngine::IsGroundBelowPlayer(float playerX, float playerY) const
{
	const float inset = 1.0f;
	const float probeDistance = 1.0f;
	const float probeWidth = m_playerCollisionWidth - inset * 2.0f;
	const float probeY = playerY - m_playerCollisionHeight * 0.5f - probeDistance * 0.5f;
	const collib::AABB groundProbe = collib::MakeAABB(playerX, probeY, probeWidth, probeDistance);
	return IsAABBBlocked(groundProbe, 0.0f);
}

int GameEngine::WorldToTileX(float worldX) const
{
	const float leftEdge = m_worldOriginX - (m_tileSize * 0.5f);
	return static_cast<int>(std::floor((worldX - leftEdge) / m_tileSize));
}

int GameEngine::WorldToTileY(float worldY) const
{
	const float topEdge = m_worldOriginY + (m_tileSize * 0.5f);
	return static_cast<int>(std::floor((topEdge - worldY) / m_tileSize));
}

float GameEngine::TileLeft(int tileX) const
{
	return m_worldOriginX + tileX * m_tileSize - (m_tileSize * 0.5f);
}

float GameEngine::TileRight(int tileX) const
{
	return m_worldOriginX + tileX * m_tileSize + (m_tileSize * 0.5f);
}

float GameEngine::TileTop(int tileY) const
{
	return m_worldOriginY - tileY * m_tileSize + (m_tileSize * 0.5f);
}

float GameEngine::TileBottom(int tileY) const
{
	return m_worldOriginY - tileY * m_tileSize - (m_tileSize * 0.5f);
}

void GameEngine::Release()
{
	DeleteInput(m_input);
	DeleteRenderer(m_renderer);
	DeleteTimer(m_timer);
	DeleteWindowInstance(m_window);
	if (m_timerResolutionRaised)
	{
		timeEndPeriod(1);
		m_timerResolutionRaised = false;
	}
}
