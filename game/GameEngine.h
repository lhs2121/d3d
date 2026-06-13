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
	struct NetworkConfig
	{
		enum class Mode
		{
			SinglePlayer,
			Host,
			Client,
		};

		Mode mode = Mode::SinglePlayer;
		std::array<char, 64> host = {};
		unsigned short port = 27015;
	};

	void Start(const char* szTitle, float x, float y, float width, float height, HINSTANCE hInstance, const NetworkConfig& networkConfig = NetworkConfig());
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
		unsigned int networkId = 0;
		int amount = 1;
		int pickupPlayerId = 0;
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

	struct RemotePlayerState
	{
		PlayerState player;
		float lastHeardTime = 0.0f;
		float attackTimer = 0.0f;
		int id = 0;
		int health = 100;
		int selectedInventorySlot = 0;
		bool active = false;
	};

	struct RemoteBlockBreakState
	{
		float progress = 0.0f;
		float lastHeardTime = 0.0f;
		int playerId = 0;
		int tileX = 0;
		int tileY = 0;
		bool active = false;
	};

	struct NetworkPeerState
	{
		sockaddr_in address = {};
		float lastHeardTime = 0.0f;
		unsigned int token = 0;
		int playerId = 0;
		bool active = false;
	};

	struct NetworkTileEditState
	{
		int tileX = 0;
		int tileY = 0;
		unsigned short tileIndex = 0;
		unsigned char visible = 0;
		unsigned int sequence = 0;
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

	struct UiRect
	{
		float left = 0.0f;
		float top = 0.0f;
		float width = 0.0f;
		float height = 0.0f;
	};

	void InitializeWorld();
	void UpdateFrameStats(float deltaTime);
	void AccumulateCpuStats(const CpuFrameStats& stats);
	void ToggleFrameLimiter();
	void ApplyFrameLimiter(const LARGE_INTEGER& frameStart);
	int GetMonitorRefreshRate() const;
	void InitializeNetwork(const NetworkConfig& config);
	bool StartNetworkHost(unsigned short port);
	bool StartNetworkClient(const char* host, unsigned short port);
	bool WaitForNetworkSeed(float timeoutSeconds);
	void ShutdownNetwork();
	void UpdateStartMenu(float deltaTime);
	void DrawStartMenu();
	void BeginSelectedGameFromMenu();
	int GetStartMenuActionAt(float viewX, float viewY) const;
	void UpdateMultiplayerMenu(float deltaTime);
	void DrawMultiplayerMenu();
	void SetMultiplayerMenuStatus(const char* text);
	void TryBeginMenuHost();
	void TryBeginMenuJoin();
	void AppendMultiplayerJoinHostChar(char value);
	void PasteMultiplayerJoinHostFromClipboard();
	int GetMultiplayerMenuActionAt(float viewX, float viewY) const;
	bool IsPointInsideRect(float pointX, float pointY, float centerX, float centerY, float width, float height) const;
	void RefreshLocalNetworkAddress();
	void PollNetwork();
	void UpdateNetwork(float deltaTime);
	void SendNetworkHello();
	void SendLocalPlayerState();
	void SendWelcomePacket(const sockaddr_in& address, int playerId, unsigned int token);
	void SendTileEditPacket(int tileX, int tileY, unsigned short tileIndex, unsigned char visible, unsigned int sequence, const sockaddr_in* targetAddress);
	void BroadcastTileEdit(int tileX, int tileY, unsigned short tileIndex, unsigned char visible);
	void PublishLocalTileEdit(int tileX, int tileY);
	void ApplyNetworkTileEdit(int tileX, int tileY, unsigned short tileIndex, unsigned char visible);
	void SendBlockBreakPacket(int playerId, int tileX, int tileY, float progress, unsigned char active, const sockaddr_in* targetAddress);
	void PublishBlockBreakState(int tileX, int tileY, float progress, bool active);
	void ApplyNetworkBlockBreakState(int playerId, int tileX, int tileY, float progress, bool active);
	void SendLeafEffectPacket(int playerId, int tileX, int tileY, unsigned int seed, const sockaddr_in* targetAddress);
	void PublishLeafBreakEffect(int tileX, int tileY, unsigned int seed);
	void ApplyNetworkLeafBreakEffect(int tileX, int tileY, unsigned int seed);
	void SendDroppedItemPacket(const DroppedItemState& item, int playerId, const sockaddr_in* targetAddress);
	void PublishDroppedItemState(const DroppedItemState& item);
	void ApplyNetworkDroppedItemState(unsigned int networkId, int playerId, int pickupPlayerId, float x, float y, float velocityX, float velocityY, float pickupDelay, unsigned short tileIndex, int amount, unsigned char alive);
	void SendOwnedDroppedItemStates();
	unsigned int GenerateNetworkItemId();
	void EnsureDroppedItemNetworkId(DroppedItemState& item);
	DroppedItemState* FindDroppedItemByNetworkId(unsigned int networkId);
	bool IsNetworkItemOwnedByLocal(const DroppedItemState& item) const;
	int ChooseDroppedItemPickupPlayer(float worldX, float worldY) const;
	bool CanLocalPlayerPickupItem(const DroppedItemState& item) const;
	bool TryGetDroppedItemPickupPosition(const DroppedItemState& item, float& targetX, float& targetY) const;
	bool SendNetworkPacket(const sockaddr_in& address, const void* packet, int packetSize);
	void BroadcastNetworkPacket(const void* packet, int packetSize, const sockaddr_in* exceptAddress = nullptr);
	void HandleNetworkPacket(const char* data, int dataSize, const sockaddr_in& from);
	int FindNetworkPeer(const sockaddr_in& address) const;
	int FindRemotePlayer(int playerId) const;
	RemotePlayerState* GetOrCreateRemotePlayer(int playerId);
	const char* GetNetworkModeText() const;
	void UpdateMapReveal();
	void RevealAllMap();
	void UpdatePlayer(float deltaTime);
	void UpdateInventoryInput();
	void UpdateCrafting(float deltaTime);
	void UpdateDebugLogInput();
	void ClampCraftingScrollOffset();
	int GetCraftingRecipeCount() const;
	int GetVisibleCraftingRecipeRows() const;
	int GetMaxCraftingScrollOffset() const;
	bool CraftSword();
	bool CraftAxe();
	bool CraftTable();
	bool CraftRecipe(int recipeIndex);
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
	void DrawBlockCrackPattern(int tileX, int tileY, float progress);
	void DrawCrackLine(float startX, float startY, float endX, float endY, float thickness, float alpha);
	void DrawLeafParticles();
	void DrawMonsters();
	void DrawDroppedItems();
	void DrawRemotePlayers();
	void DrawPlayerAttackMotion();
	void DrawAttackArc();
	void DrawInventory();
	void DrawWeaponIcon(float centerX, float centerY, float size, float depth, float alpha, int slot);
	void DrawMinimap();
	void DrawPlayerStatus();
	void DrawPlayerStatsPanel();
	void DrawNetworkStatus();
	void DrawEquipmentTooltip();
	void DrawCraftingPanel();
	void DrawDebugLogPanel();
	void DrawUiShell();
	void DrawUiPanel(const UiRect& rect, const char* title, float depth = -6.2f);
	void DrawClassicUiPanel(const UiRect& rect, const char* title, float depth = -6.2f);
	void DrawUiText(float x, float y, const char* text, float pixelSize, float colorR, float colorG, float colorB, float colorA = 1.0f, float depth = -8.0f);
	void DrawCenteredUiText(float centerX, float y, const char* text, float pixelSize, float colorR, float colorG, float colorB, float colorA = 1.0f, float depth = -8.0f);
	float DrawUiWrappedText(float x, float y, const char* text, float pixelSize, float maxWidth, float lineHeight, float colorR, float colorG, float colorB, float colorA = 1.0f, float depth = -8.0f);
	float GetUiTextWidth(const char* text, float pixelSize) const;
	void DrawSolidRect(float centerX, float centerY, float width, float height, float colorR, float colorG, float colorB, float colorA, float depth);
	void DrawText(float x, float y, const char* text, float pixelSize, float colorR, float colorG, float colorB, float colorA, float depth);
	void DrawGlyph(float x, float y, char glyph, float pixelSize, float colorR, float colorG, float colorB, float colorA, float depth);
	CraftingPanelLayout GetCraftingPanelLayout() const;
	UiRect GetGameViewportRect() const;
	UiRect GetInventoryPanelRect() const;
	UiRect GetLogPanelRect() const;
	UiRect GetRightPanelRect(int panelIndex) const;
	bool GetCursorViewPosition(float& viewX, float& viewY) const;
	bool GetCursorGameViewPosition(float& viewX, float& viewY) const;
	int GetCraftingRecipeAt(float viewX, float viewY) const;
	int GetInventorySlotAt(float viewX, float viewY) const;
	bool IsCursorOverCraftingPanel() const;
	bool GetHoveredTile(int& tileX, int& tileY) const;
	bool GetHoveredBlockTile(int& tileX, int& tileY) const;
	collib::AABB GetPlayerAABB(float playerX, float playerY) const;
	collib::AABB GetMonsterAABB(float monsterX, float monsterY) const;
	collib::AABB GetTileAABB(int tileX, int tileY) const;
	bool IsWindowFocused() const;
	bool IsKeyDown(int keyCode) const;
	bool IsKeyHeld(int keyCode);
	bool IsTileInBounds(int tileX, int tileY) const;
	bool IsSolidTile(int tileX, int tileY) const;
	bool IsTileNearPlayer(int tileX, int tileY, float maxTiles) const;
	bool IsInventoryBlockSlot(int slot) const;
	bool IsInventoryWeaponSlot(int slot) const;
	bool IsMapTileRevealed(int tileX, int tileY) const;
	bool CanCraftTable() const;
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
	void SpawnLeafBreakEffectWithSeed(int tileX, int tileY, unsigned int seed);
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
	std::vector<RemotePlayerState> m_remotePlayers;
	std::vector<RemoteBlockBreakState> m_remoteBlockBreaks;
	std::vector<NetworkPeerState> m_networkPeers;
	std::vector<NetworkTileEditState> m_networkTileHistory;
	std::vector<NetworkTileEditState> m_pendingNetworkTileEdits;
	std::vector<DroppedItemState> m_pendingNetworkDroppedItems;
	std::array<int, InventorySlotCount> m_inventoryCounts = {};
	std::array<char, 64> m_multiplayerJoinHost = { '1', '2', '7', '.', '0', '.', '0', '.', '1', '\0' };
	std::array<char, 64> m_multiplayerMenuStatus = {};
	std::array<char, 48> m_localNetworkAddress = {};
	PlayerState m_player;
	NetworkConfig m_networkConfig;
	NetworkConfig::Mode m_networkMode = NetworkConfig::Mode::SinglePlayer;
	SOCKET m_networkSocket = INVALID_SOCKET;
	sockaddr_in m_networkServerAddress = {};
	int m_selectedInventorySlot = 0;
	int m_inventoryWheelRemainder = 0;
	int m_craftingWheelRemainder = 0;
	int m_craftingScrollOffset = 0;
	int m_localPlayerId = 1;
	int m_nextNetworkPlayerId = 2;
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
	float m_networkTime = 0.0f;
	float m_networkSendTimer = 0.0f;
	float m_networkHelloTimer = 0.0f;
	float m_networkItemSendTimer = 0.0f;
	float m_networkStatusTimer = 0.0f;
	CpuFrameStats m_cpuStatsAccum;
	CpuFrameStats m_displayCpuStats;
	int m_frameStatsCount = 0;
	int m_cpuStatsCount = 0;
	int m_networkBreakingBlockIndex = -1;
	int m_playerHealth = 100;
	int m_targetRefreshRate = 60;
	unsigned int m_worldSeed = 0;
	unsigned int m_networkClientToken = 0;
	unsigned int m_networkPlayerSequence = 0;
	unsigned int m_networkTileSequence = 0;
	unsigned int m_networkItemSequence = 0;
	unsigned int m_blockGridVersion = 1;
	unsigned int m_blockChunkVersionCounter = 1;
	bool m_winsockStarted = false;
	bool m_networkConnected = false;
	bool m_networkSeedReady = false;
	bool m_gameStarted = false;
	bool m_multiplayerMenuOpen = false;
	bool m_startJoinHostEditing = false;
	bool m_multiplayerJoinHostEditing = false;
	bool m_acceptInput = false;
	bool m_showRenderStats = false;
	bool m_frameLimitEnabled = false;
	bool m_timerResolutionRaised = false;
	bool m_uiConsumesLeftMouse = false;
	bool m_minimapExpanded = false;
	bool m_debugRevealMap = false;
	bool m_cachedMinimapExpanded = false;
	bool m_minimapDirty = true;
	std::vector<unsigned int> m_blockChunkVersions;
	std::array<char, 48> m_statusText = {};
	float m_statusTextTimer = 0.0f;
};
