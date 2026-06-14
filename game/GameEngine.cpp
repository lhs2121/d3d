#include "pch.h"
#include "GameEngine.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mmsystem.h>
#include <random>
#include <string>

#pragma comment(lib, "winmm.lib")

namespace
{
	constexpr int KeyA = 0x41;
	constexpr int KeyD = 0x44;
	constexpr int KeyS = 0x53;
	constexpr int KeyJump = VK_SPACE;
	constexpr int KeyMinimap = VK_TAB;
	constexpr float BlockBreakDuration = 0.85f;
	constexpr float BlockPlaceRepeatInterval = 0.085f;
	constexpr float InteractionRangeTiles = 6.0f;
	constexpr unsigned int NetworkMagic = 0x4433444Du;
	constexpr unsigned char NetworkVersion = 1;
	constexpr float NetworkPlayerSendInterval = 1.0f / 30.0f;
	constexpr float NetworkItemSendInterval = 0.12f;
	constexpr float NetworkHelloInterval = 0.25f;
	constexpr float NetworkRemoteTimeout = 5.0f;
	constexpr float NetworkBlockBreakRemoteTimeout = 0.35f;
	constexpr int NetworkMaxPeers = 8;
	constexpr int MultiplayerActionNone = 0;
	constexpr int MultiplayerActionHost = 1;
	constexpr int MultiplayerActionJoin = 2;
	constexpr int MultiplayerActionInput = 3;
	constexpr int MultiplayerActionClose = 4;
	constexpr int MultiplayerActionCopyIp = 5;
	constexpr int MultiplayerActionNickname = 6;
	constexpr int StartMenuActionSingle = 11;
	constexpr int StartMenuActionHost = 12;
	constexpr int StartMenuActionJoin = 13;
	constexpr int StartMenuActionInput = 14;
	constexpr int StartMenuActionStart = 15;
	constexpr int StartMenuActionNickname = 16;
	constexpr int NetworkNicknameLength = 24;
	constexpr const WCHAR* BlockAtlasTexture = L"assets\\texture\\block_atlas_extended.png";
	constexpr const WCHAR* UiPanelDefaultTexture = L"assets\\ui\\panel_default.png";
	constexpr const WCHAR* UiPanelInventoryTexture = L"assets\\ui\\panel_inventory.png";
	constexpr const WCHAR* UiPanelLogTexture = L"assets\\ui\\panel_log.png";
	constexpr const WCHAR* UiPanelMapTexture = L"assets\\ui\\panel_map.png";
	constexpr const WCHAR* UiPanelCraftTexture = L"assets\\ui\\panel_craft.png";
	constexpr const WCHAR* UiPanelStatusTexture = L"assets\\ui\\panel_status.png";
	constexpr const WCHAR* UiPanelNetworkTexture = L"assets\\ui\\panel_network.png";
	constexpr const WCHAR* UiPanelTooltipTexture = L"assets\\ui\\panel_tooltip.png";
	constexpr const WCHAR* UiPanelGameTexture = L"assets\\ui\\panel_game.png";
	constexpr const WCHAR* UiPanelTextures[] =
	{
		UiPanelDefaultTexture,
		UiPanelInventoryTexture,
		UiPanelLogTexture,
		UiPanelMapTexture,
		UiPanelCraftTexture,
		UiPanelStatusTexture,
		UiPanelNetworkTexture,
		UiPanelTooltipTexture,
		UiPanelGameTexture,
	};
	constexpr int BlockAtlasColumns = 16;
	constexpr int BlockAtlasRows = 1;
	constexpr float UiThemeCreamR = 0.92f;
	constexpr float UiThemeCreamG = 0.96f;
	constexpr float UiThemeCreamB = 0.92f;
	constexpr float UiThemeMutedR = 0.36f;
	constexpr float UiThemeMutedG = 0.48f;
	constexpr float UiThemeMutedB = 0.46f;
	constexpr float UiThemeBodyR = 0.006f;
	constexpr float UiThemeBodyG = 0.010f;
	constexpr float UiThemeBodyB = 0.012f;
	constexpr float UiThemeBodyDarkR = 0.002f;
	constexpr float UiThemeBodyDarkG = 0.004f;
	constexpr float UiThemeBodyDarkB = 0.006f;
	constexpr float UiThemeShadowR = 0.0f;
	constexpr float UiThemeShadowG = 0.0f;
	constexpr float UiThemeShadowB = 0.0f;

	const WCHAR* GetUiPanelTextureFile(const char* title)
	{
		if (title == nullptr || title[0] == '\0')
			return UiPanelDefaultTexture;
		if (std::strcmp(title, "inventory") == 0)
			return UiPanelInventoryTexture;
		if (std::strcmp(title, "log") == 0 || std::strcmp(title, "performance") == 0)
			return UiPanelLogTexture;
		if (std::strcmp(title, "map") == 0)
			return UiPanelMapTexture;
		if (std::strcmp(title, "craft") == 0)
			return UiPanelCraftTexture;
		if (std::strcmp(title, "status") == 0)
			return UiPanelStatusTexture;
		if (std::strcmp(title, "network") == 0)
			return UiPanelNetworkTexture;
		if (std::strcmp(title, "tooltip") == 0)
			return UiPanelTooltipTexture;
		if (std::strcmp(title, "game") == 0)
			return UiPanelGameTexture;

		return UiPanelDefaultTexture;
	}

	struct BackgroundImageLayer
	{
		const WCHAR* textureFile = nullptr;
		float alpha = 1.0f;
		float parallaxX = 0.0f;
		float parallaxY = 0.0f;
		float depth = 5.0f;
	};

	enum class NetworkPacketType : unsigned char
	{
		Hello = 1,
		Welcome = 2,
		PlayerState = 3,
		TileEdit = 4,
		BlockBreak = 5,
		LeafEffect = 6,
		DroppedItem = 7,
	};

#pragma pack(push, 1)
	struct NetworkPacketHeader
	{
		unsigned int magic = NetworkMagic;
		unsigned char version = NetworkVersion;
		unsigned char type = 0;
		unsigned short size = 0;
	};

	struct NetworkHelloPacket
	{
		NetworkPacketHeader header;
		unsigned int token = 0;
	};

	struct NetworkWelcomePacket
	{
		NetworkPacketHeader header;
		unsigned int token = 0;
		int playerId = 0;
		unsigned int worldSeed = 0;
		int blockWidth = 0;
		int blockHeight = 0;
		float spawnX = 0.0f;
		float spawnY = 0.0f;
	};

	struct NetworkPlayerStatePacket
	{
		NetworkPacketHeader header;
		unsigned int sequence = 0;
		int playerId = 0;
		float x = 0.0f;
		float y = 0.0f;
		float velocityX = 0.0f;
		float velocityY = 0.0f;
		float animationTime = 0.0f;
		float attackTimer = 0.0f;
		int facing = 1;
		int health = 100;
		int selectedInventorySlot = 0;
		unsigned char onGround = 0;
		char nickname[NetworkNicknameLength] = {};
	};

	struct NetworkTileEditPacket
	{
		NetworkPacketHeader header;
		unsigned int sequence = 0;
		int playerId = 0;
		int tileX = 0;
		int tileY = 0;
		unsigned short tileIndex = 0;
		unsigned char visible = 0;
	};

	struct NetworkBlockBreakPacket
	{
		NetworkPacketHeader header;
		int playerId = 0;
		int tileX = 0;
		int tileY = 0;
		float progress = 0.0f;
		unsigned char active = 0;
	};

	struct NetworkLeafEffectPacket
	{
		NetworkPacketHeader header;
		int playerId = 0;
		int tileX = 0;
		int tileY = 0;
		unsigned int seed = 0;
	};

	struct NetworkDroppedItemPacket
	{
		NetworkPacketHeader header;
		int playerId = 0;
		unsigned int networkId = 0;
		float x = 0.0f;
		float y = 0.0f;
		float velocityX = 0.0f;
		float velocityY = 0.0f;
		float pickupDelay = 0.0f;
		unsigned short tileIndex = 0;
		int amount = 0;
		int pickupPlayerId = 0;
		unsigned char alive = 0;
	};
#pragma pack(pop)

	NetworkPacketHeader MakeNetworkHeader(NetworkPacketType type, size_t packetSize)
	{
		NetworkPacketHeader header;
		header.magic = NetworkMagic;
		header.version = NetworkVersion;
		header.type = static_cast<unsigned char>(type);
		header.size = static_cast<unsigned short>(packetSize);
		return header;
	}

	bool IsHostInputCharacter(char value)
	{
		return (value >= '0' && value <= '9') ||
			(value >= 'A' && value <= 'Z') ||
			(value >= 'a' && value <= 'z') ||
			value == '.' ||
			value == '-';
	}

	bool IsNicknameInputCharacter(char value)
	{
		return (value >= '0' && value <= '9') ||
			(value >= 'A' && value <= 'Z') ||
			(value >= 'a' && value <= 'z') ||
			value == '-' ||
			value == '_' ||
			value == ' ';
	}

	bool CopyTextToClipboard(const char* text)
	{
		if (text == nullptr || text[0] == '\0')
			return false;

		if (!OpenClipboard(nullptr))
			return false;

		EmptyClipboard();
		const size_t textLength = std::strlen(text) + 1;
		HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, textLength);
		if (memory == nullptr)
		{
			CloseClipboard();
			return false;
		}

		void* buffer = GlobalLock(memory);
		if (buffer == nullptr)
		{
			GlobalFree(memory);
			CloseClipboard();
			return false;
		}

		memcpy(buffer, text, textLength);
		GlobalUnlock(memory);
		SetClipboardData(CF_TEXT, memory);
		CloseClipboard();
		return true;
	}
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
	constexpr int PlayerBaseMaxHealth = 100;
	constexpr int PlayerBaseAttack = 8;
	constexpr int PlayerBaseDefense = 1;
	constexpr float PlayerMoveSpeedTiles = 6.8f;
	constexpr float PlayerJumpSpeedTiles = 15.4f;
	constexpr float PlayerAttackRange = 42.0f;
	constexpr float PlayerAttackHeight = 40.0f;
	constexpr float PlayerAttackDuration = 0.18f;
	constexpr float PlayerHurtFlashDuration = 0.55f;
	constexpr float PlayerKnockbackDuration = 0.28f;
	constexpr float PlayerKnockbackCooldownDuration = 1.00f;

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
		BlockPlacedWood = 16,
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

	enum class InventoryItemKind
	{
		Empty,
		TerrainBlock,
		Furniture,
		Equipment,
	};

	struct EquipmentStats
	{
		const char* name = "";
		const char* role = "";
		int attackBonus = 0;
		int defenseBonus = 0;
		float chopSpeedMultiplier = 1.0f;
	};

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
	constexpr unsigned short BlockAtlasTileRemap[] =
	{
		BlockGrass,
		BlockDirt,
		BlockStone,
		BlockOre,
		BlockSand,
		BlockWood,
		BlockLeaves,
		BlockCraftingTable,
		BlockPrairieStone,
		BlockMossStone,
		BlockSandstone,
		BlockDesertStone,
		BlockSnow,
		BlockIce,
		BlockFrozenStone,
		BlockCrystalOre,
		BlockWood,
	};

	EquipmentStats GetEquipmentStatsForSlot(int slot)
	{
		if (slot == SlotSword)
			return { "검", "전투", 28, 0, 1.0f };
		if (slot == SlotAxe)
			return { "도끼", "채집", 4, 0, 4.5f };

		return {};
	}

	InventoryItemKind GetInventoryItemKind(int slot)
	{
		if (slot == SlotCraftingTable)
			return InventoryItemKind::Furniture;
		if (slot >= 0 && slot < BlockInventorySlotCount)
			return InventoryItemKind::TerrainBlock;
		if (slot == SlotSword || slot == SlotAxe)
			return InventoryItemKind::Equipment;

		return InventoryItemKind::Empty;
	}

	bool IsTopSurfaceOnlyItem(int slot)
	{
		return GetInventoryItemKind(slot) == InventoryItemKind::Furniture;
	}

	unsigned short GetPlacementTileIndexForSlot(int slot)
	{
		if (slot == SlotWood)
			return BlockPlacedWood;
		if (slot >= 0 && slot < BlockInventorySlotCount)
			return InventoryTileIndices[slot];

		return BlockGrass;
	}

	unsigned short GetDroppedItemTileIndex(unsigned short tileIndex)
	{
		if (tileIndex == BlockPlacedWood)
			return BlockWood;

		return tileIndex;
	}

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

	constexpr const WCHAR* PlayerIdleTexture = L"assets\\player\\Sprites\\Idle.png";
	constexpr const WCHAR* PlayerRunTexture = L"assets\\player\\Sprites\\Run.png";
	constexpr const WCHAR* PlayerJumpTexture = L"assets\\player\\Sprites\\Jump.png";
	constexpr const WCHAR* PlayerFallTexture = L"assets\\player\\Sprites\\Fall.png";
	constexpr const WCHAR* PlayerAttackTexture = L"assets\\player\\Sprites\\Attack1.png";
	constexpr const WCHAR* PlayerHitTexture = L"assets\\player\\Sprites\\Take Hit.png";
	constexpr const WCHAR* PlayerSpriteTextures[] =
	{
		PlayerIdleTexture,
		PlayerRunTexture,
		PlayerJumpTexture,
		PlayerFallTexture,
		PlayerAttackTexture,
		PlayerHitTexture,
	};
	constexpr int PlayerIdleFrames = 8;
	constexpr int PlayerRunFrames = 8;
	constexpr int PlayerJumpFrames = 2;
	constexpr int PlayerFallFrames = 2;
	constexpr int PlayerAttackFrames = 4;
	constexpr int PlayerHitFrames = 4;
	constexpr int PlayerSpriteRows = 1;
	constexpr float PlayerSpriteFramePixelHeight = 150.0f;
	constexpr float PlayerSpriteFootPixelY = 95.0f;
	struct MonsterVisualDesc
	{
		const WCHAR* idleTexture = nullptr;
		int idleFrames = 1;
		const WCHAR* moveTexture = nullptr;
		int moveFrames = 1;
		const WCHAR* hurtTexture = nullptr;
		int hurtFrames = 1;
		float drawSize = 48.0f;
		float yOffset = 4.0f;
	};
	constexpr MonsterVisualDesc MonsterVisuals[BackgroundBiomeCount] =
	{
		{
			L"assets\\monster\\Sprites\\Slime\\idle.png", 14,
			L"assets\\monster\\Sprites\\Slime\\walk.png", 6,
			L"assets\\monster\\Sprites\\Slime\\hurt.png", 3,
			160.0f, -6.0f,
		},
		{
			L"assets\\monster\\Sprites\\Rat\\idle.png", 10,
			L"assets\\monster\\Sprites\\Rat\\run.png", 8,
			L"assets\\monster\\Sprites\\Rat\\hurt.png", 3,
			72.0f, -5.0f,
		},
		{
			L"assets\\monster\\Sprites\\Bat\\fly.png", 11,
			L"assets\\monster\\Sprites\\Bat\\fly.png", 11,
			L"assets\\monster\\Sprites\\Bat\\hurt.png", 3,
			84.0f, 8.0f,
		},
	};
	constexpr int MonsterSpriteRows = 1;

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
		{ '/', { 0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x10 } },
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

void GameEngine::Start(const char* szTitle, float x, float y, float width, float height, HINSTANCE hInstance, const NetworkConfig& networkConfig)
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
	for (const WCHAR* uiPanelTexture : UiPanelTextures)
		m_renderer->LoadTexture(uiPanelTexture);
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
	for (const WCHAR* playerTexture : PlayerSpriteTextures)
		m_renderer->LoadTexture(playerTexture);
	for (const MonsterVisualDesc& monsterVisual : MonsterVisuals)
	{
		m_renderer->LoadTexture(monsterVisual.idleTexture);
		m_renderer->LoadTexture(monsterVisual.moveTexture);
		m_renderer->LoadTexture(monsterVisual.hurtTexture);
	}

	m_networkConfig = networkConfig;
	m_networkMode = NetworkConfig::Mode::SinglePlayer;
	m_gameStarted = false;
	m_multiplayerMenuOpen = false;
	m_startJoinHostEditing = networkConfig.mode == NetworkConfig::Mode::Client;
	if (networkConfig.mode == NetworkConfig::Mode::Client && networkConfig.host[0] != '\0')
		std::snprintf(m_multiplayerJoinHost.data(), m_multiplayerJoinHost.size(), "%s", networkConfig.host.data());
	SetMultiplayerMenuStatus(networkConfig.mode == NetworkConfig::Mode::Host ? "호스트 선택됨" :
		(networkConfig.mode == NetworkConfig::Mode::Client ? "참가 선택됨" : "모드를 고르고 시작"));

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

	if (m_renderer != nullptr && m_window != nullptr && m_window->GetWidth() > 0.0f && m_window->GetHeight() > 0.0f)
	{
		m_renderer->Resize(
			static_cast<UINT>(m_window->GetWidth()),
			static_cast<UINT>(m_window->GetHeight()));
	}

	UpdateFrameStats(deltaTime);
	m_input->Update();
	m_acceptInput = IsWindowFocused();
	m_uiConsumesLeftMouse = false;

	if (!m_gameStarted)
	{
		if (m_acceptInput)
			UpdateStartMenu(deltaTime);
		UpdateNetwork(deltaTime);

		QueryPerformanceCounter(&sectionEnd);
		cpuStats.inputMs = CounterMilliseconds(sectionStart, sectionEnd);
		cpuStats.itemsMs = cpuStats.inputMs;
		sectionStart = sectionEnd;

		m_renderer->BeginFrame();
		LARGE_INTEGER drawStart = {};
		LARGE_INTEGER drawEnd = {};
		QueryPerformanceCounter(&drawStart);
		DrawStartMenu();
		QueryPerformanceCounter(&drawEnd);
		m_renderer->EndFrame();

		QueryPerformanceCounter(&sectionEnd);
		cpuStats.drawWorldMs = CounterMilliseconds(drawStart, drawEnd);
		cpuStats.renderMs = CounterMilliseconds(sectionStart, sectionEnd);
		cpuStats.totalMs = CounterMilliseconds(frameStart, sectionEnd);
		AccumulateCpuStats(cpuStats);
		ApplyFrameLimiter(frameStart);
		return;
	}

	if (IsKeyDown(KeyMinimap))
	{
		m_minimapExpanded = !m_minimapExpanded;
		m_minimapDirty = true;
	}
	PollNetwork();

	QueryPerformanceCounter(&sectionEnd);
	cpuStats.inputMs = CounterMilliseconds(sectionStart, sectionEnd);
	sectionStart = sectionEnd;

	UpdateInventoryInput();
	UpdateCrafting(deltaTime);
	UpdateDebugLogInput();
	TryPlaceSelectedBlock(deltaTime);
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
	UpdateLeafParticles(deltaTime);
	UpdateNetwork(deltaTime);

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

#include "GameEngineWorldGeneration.inl"

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
		std::snprintf(text, sizeof(text), "프레임 제한 %d", m_targetRefreshRate);
	else
		std::snprintf(text, sizeof(text), "프레임 제한 끔");

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

#include "GameEngineGameplay.inl"

#include "GameEngineRender.inl"

#include "GameEngineNetwork.inl"

#include "GameEngineWorldQueries.inl"

void GameEngine::Release()
{
	ShutdownNetwork();
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
