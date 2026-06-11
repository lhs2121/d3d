#pragma once
#include <Windows.h>
#include <commonlib/Interface.h>
#include <inputlib/Interface.h>
#include <collib/Interface.h>
#include <rendererlib/Interface.h>
#include <windowlib/Interface.h>
#include <array>
#include <vector>

class GameEngine : public IGameLoop
{
public:
	void Start(const char* szTitle, float x, float y, float width, float height, HINSTANCE hInstance);
	void Update() override;
	void Release() override;

private:
	struct PlayerState
	{
		float x = 0.0f;
		float y = 0.0f;
		float velocityX = 0.0f;
		float velocityY = 0.0f;
		float animationTime = 0.0f;
		int facing = 1;
		bool onGround = false;
	};

	struct BlockBreakState
	{
		float progress = 0.0f;
		float idleTime = 0.0f;
		unsigned char active = 0;
	};

	struct MonsterState
	{
		float x = 0.0f;
		float y = 0.0f;
		float velocityY = 0.0f;
		float hurtTimer = 0.0f;
		float attackCooldown = 0.0f;
		float aiTimer = 0.0f;
		float idleTimer = 0.0f;
		float jumpCooldown = 0.0f;
		float stuckTimer = 0.0f;
		float animationTime = 0.0f;
		float contactTimer = 0.0f;
		float homeX = 0.0f;
		float lastX = 0.0f;
		int facing = -1;
		int contactDirection = 0;
		int health = 40;
		int maxHealth = 40;
		unsigned char biome = 0;
		bool onGround = false;
		bool alive = true;
		bool underground = false;
	};

	struct DroppedItemState
	{
		float x = 0.0f;
		float y = 0.0f;
		float velocityX = 0.0f;
		float velocityY = 0.0f;
		float pickupDelay = 0.0f;
		unsigned short tileIndex = 0;
		int amount = 1;
		bool alive = false;
	};

	struct LeafParticleState
	{
		float x = 0.0f;
		float y = 0.0f;
		float velocityX = 0.0f;
		float velocityY = 0.0f;
		float age = 0.0f;
		float lifetime = 0.0f;
		float rotation = 0.0f;
		float angularVelocity = 0.0f;
		float size = 0.0f;
		bool alive = false;
	};

	struct MinimapRunState
	{
		float x = 0.0f;
		float y = 0.0f;
		float width = 0.0f;
		float colorR = 0.0f;
		float colorG = 0.0f;
		float colorB = 0.0f;
	};

	struct CpuFrameStats
	{
		float totalMs = 0.0f;
		float inputMs = 0.0f;
		float simulationMs = 0.0f;
		float monstersMs = 0.0f;
		float itemsMs = 0.0f;
		float renderMs = 0.0f;
		float drawWorldMs = 0.0f;
	};

	struct CraftingPanelLayout
	{
		float left = 0.0f;
		float top = 0.0f;
		float width = 0.0f;
		float height = 0.0f;
		float rowHeight = 0.0f;
		float firstRowCenterY = 0.0f;
	};

	void InitializeWorld();
	void UpdateFrameStats(float deltaTime);
	void AccumulateCpuStats(const CpuFrameStats& stats);
	void ToggleFrameLimiter();
	void ApplyFrameLimiter(const LARGE_INTEGER& frameStart);
	int GetMonitorRefreshRate() const;
	void UpdateMapReveal();
	void RevealAllMap();
	void UpdatePlayer(float deltaTime);
	void UpdateInventoryInput();
	void UpdateCrafting(float deltaTime);
	bool CraftSword();
	bool CraftAxe();
	bool CraftFirstAvailableAtTable();
	void TryPlaceSelectedBlock();
	void UpdatePlayerCombat(float deltaTime);
	void TryPlayerAttack();
	void UpdateBlockBreaking(float deltaTime);
	void UpdateMonsters(float deltaTime);
	void UpdateDroppedItems(float deltaTime);
	void UpdateLeafParticles(float deltaTime);
	void DrawWorld();
	void DrawBackground();
	void DrawBackgroundImageLayer(const WCHAR* textureFile, float alpha, float parallaxX, float parallaxY, float depth);
	void DrawSkyBackground(int biome, float alpha);
	void DrawUndergroundBackground(float undergroundDepth, int biome, float alpha);
	void DrawMountainLayer(float baseY, float height, float spacing, float parallaxX, float parallaxY, float colorR, float colorG, float colorB, float colorA, float depth);
	void DrawCloudLayer(float y, float spacing, float parallaxX, float parallaxY, float colorR, float colorG, float colorB, float colorA, float depth);
	void DrawCaveWallLayer(float baseY, float spacing, float parallaxX, float parallaxY, float colorR, float colorG, float colorB, float colorA, float depth);
	void DrawHoveredBlockOutline();
	void DrawBlockCracks();
	void DrawCrackLine(float startX, float startY, float endX, float endY, float thickness, float alpha);
	void DrawLeafParticles();
	void DrawMonsters();
	void DrawDroppedItems();
	void DrawPlayerAttackMotion();
	void DrawAttackArc();
	void DrawInventory();
	void DrawWeaponIcon(float centerX, float centerY, float size, float depth, float alpha, int slot);
	void DrawMinimap();
	void DrawFrameStats();
	void DrawRenderStatsOverlay();
	void DrawPlayerStatus();
	void DrawPlayerStatsPanel();
	void DrawEquipmentTooltip();
	void DrawCraftingPanel();
	void DrawCraftingPrompt();
	void DrawSolidRect(float centerX, float centerY, float width, float height, float colorR, float colorG, float colorB, float colorA, float depth);
	void DrawText(float x, float y, const char* text, float pixelSize, float colorR, float colorG, float colorB, float colorA, float depth);
	void DrawGlyph(float x, float y, char glyph, float pixelSize, float colorR, float colorG, float colorB, float colorA, float depth);
	CraftingPanelLayout GetCraftingPanelLayout() const;
	bool GetCursorViewPosition(float& viewX, float& viewY) const;
	int GetCraftingRecipeAt(float viewX, float viewY) const;
	int GetInventorySlotAt(float viewX, float viewY) const;
	bool IsCursorOverCraftingPanel() const;
	bool GetHoveredTile(int& tileX, int& tileY) const;
	bool GetHoveredBlockTile(int& tileX, int& tileY) const;
	collib::AABB GetPlayerAABB(float playerX, float playerY) const;
	collib::AABB GetMonsterAABB(float monsterX, float monsterY) const;
	collib::AABB GetTileAABB(int tileX, int tileY) const;
	bool IsKeyHeld(int keyCode);
	bool IsTileInBounds(int tileX, int tileY) const;
	bool IsSolidTile(int tileX, int tileY) const;
	bool IsTileNearPlayer(int tileX, int tileY, float maxTiles) const;
	bool IsInventoryBlockSlot(int slot) const;
	bool IsInventoryWeaponSlot(int slot) const;
	bool IsMapTileRevealed(int tileX, int tileY) const;
	bool CanCraftSword() const;
	bool CanCraftAxe() const;
	bool CanPlaceBlockAt(int tileX, int tileY) const;
	bool CanPlaceSelectedBlockAt(int tileX, int tileY) const;
	bool ShouldLeftClickAttack() const;
	bool IsCraftingTableNearby() const;
	bool TryHarvestTreeAt(int tileX, int tileY);
	float GetBlockBreakDuration(unsigned short tileIndex) const;
	int GetPlayerMaxHealth() const;
	int GetPlayerDefense() const;
	int GetPlayerAttackDamage() const;
	float GetPlayerMoveSpeedTiles() const;
	float GetPlayerJumpSpeedTiles() const;
	float GetSelectedChopSpeedMultiplier() const;
	void SpawnLeafBreakEffect(int tileX, int tileY);
	bool IsGroundBelowBox(float centerX, float centerY, float width, float height) const;
	void SetStatusText(const char* text, float duration);
	void AddBlockToInventory(unsigned short tileIndex, int amount);
	void SpawnDroppedItem(float worldX, float worldY, unsigned short tileIndex, int amount, bool mergeNearby = true);
	void InitializeBlockChunkCache();
	void MarkBlockChunkDirty(int tileX, int tileY);
	void MarkBlockIndexDirty(int blockIndex);
	void RebuildMonsterSpatialGrid();
	void QueryMonstersInAABB(const collib::AABB& area, std::vector<int>& results) const;
	float GetViewHalfWidth() const;
	float GetViewHalfHeight() const;
	float GetSurfaceWorldYAt(float worldX) const;
	bool IsAABBBlocked(const collib::AABB& box, float inset) const;
	bool IsGroundBelowPlayer(float playerX, float playerY) const;
	int WorldToTileX(float worldX) const;
	int WorldToTileY(float worldY) const;
	float TileLeft(int tileX) const;
	float TileRight(int tileX) const;
	float TileTop(int tileY) const;
	float TileBottom(int tileY) const;

	static constexpr int InventorySlotCount = 10;

	IRenderer* m_renderer = nullptr;
	ITimer* m_timer = nullptr;
	IWindow* m_window = nullptr;
	IInput* m_input = nullptr;
	std::vector<BlockTile> m_blocks;
	std::vector<BlockBreakState> m_blockBreaks;
	std::vector<int> m_surfaceHeights;
	std::vector<unsigned char> m_biomes;
	std::vector<unsigned char> m_revealedTiles;
	std::vector<MonsterState> m_monsters;
	std::vector<DroppedItemState> m_droppedItems;
	std::vector<LeafParticleState> m_leafParticles;
	std::vector<int> m_monsterGridHeads;
	std::vector<int> m_monsterGridNext;
	std::vector<int> m_monsterQueryScratch;
	std::vector<int> m_monsterOverlapScratch;
	std::vector<MinimapRunState> m_minimapRuns;
	std::array<int, InventorySlotCount> m_inventoryCounts = {};
	PlayerState m_player;
	int m_selectedInventorySlot = 0;
	int m_inventoryWheelRemainder = 0;
	int m_blockWidth = 720;
	int m_blockHeight = 128;
	int m_monsterGridColumns = 0;
	int m_monsterGridRows = 0;
	int m_monsterGridCellTiles = 8;
	int m_blockChunkSizeTiles = 16;
	int m_blockChunkColumns = 0;
	int m_blockChunkRows = 0;
	int m_cachedMinimapStartX = -1;
	int m_cachedMinimapEndX = -1;
	int m_cachedMinimapStartY = -1;
	int m_cachedMinimapEndY = -1;
	int m_cachedMinimapSampleStep = -1;
	int m_surfaceRow = 28;
	float m_tileSize = 16.0f;
	float m_worldOriginX = -1024.0f;
	float m_worldOriginY = 1040.0f;
	float m_cameraX = 0.0f;
	float m_cameraY = 0.0f;
	float m_playerCollisionWidth = 16.0f;
	float m_playerCollisionHeight = 32.0f;
	float m_playerDrawWidth = 16.0f;
	float m_playerDrawHeight = 32.0f;
	float m_playerSpawnX = 0.0f;
	float m_playerSpawnY = 0.0f;
	float m_playerInvulnerableTimer = 0.0f;
	float m_playerHurtFlashTimer = 0.0f;
	float m_playerKnockbackTimer = 0.0f;
	float m_playerKnockbackCooldownTimer = 0.0f;
	float m_attackCooldown = 0.0f;
	float m_attackTimer = 0.0f;
	float m_frameStatsTimer = 0.0f;
	float m_displayFps = 0.0f;
	float m_displayFrameMs = 0.0f;
	CpuFrameStats m_cpuStatsAccum;
	CpuFrameStats m_displayCpuStats;
	int m_frameStatsCount = 0;
	int m_cpuStatsCount = 0;
	int m_playerHealth = 100;
	int m_targetRefreshRate = 60;
	unsigned int m_blockGridVersion = 1;
	unsigned int m_blockChunkVersionCounter = 1;
	bool m_showRenderStats = false;
	bool m_frameLimitEnabled = false;
	bool m_timerResolutionRaised = false;
	bool m_uiConsumesLeftMouse = false;
	bool m_minimapExpanded = false;
	bool m_debugRevealMap = false;
	bool m_showPlayerStats = false;
	bool m_cachedMinimapExpanded = false;
	bool m_minimapDirty = true;
	std::vector<unsigned int> m_blockChunkVersions;
	std::array<char, 48> m_statusText = {};
	float m_statusTextTimer = 0.0f;
};
