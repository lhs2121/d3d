#pragma once
#include <Windows.h>
#include <commonlib/Interface.h>
#include <inputlib/Interface.h>
#include <collib/Interface.h>
#include <rendererlib/Interface.h>
#include <windowlib/Interface.h>
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

	void InitializeWorld();
	void UpdatePlayer(float deltaTime);
	void UpdateBlockBreaking(float deltaTime);
	void DrawWorld();
	void DrawHoveredBlockOutline();
	void DrawBlockCracks();
	void DrawCrackLine(float startX, float startY, float endX, float endY, float thickness, float alpha);
	bool GetHoveredBlockTile(int& tileX, int& tileY) const;
	collib::AABB GetPlayerAABB(float playerX, float playerY) const;
	collib::AABB GetTileAABB(int tileX, int tileY) const;
	bool IsKeyHeld(int keyCode);
	bool IsSolidTile(int tileX, int tileY) const;
	bool IsAABBBlocked(const collib::AABB& box, float inset) const;
	bool IsGroundBelowPlayer(float playerX, float playerY) const;
	int WorldToTileX(float worldX) const;
	int WorldToTileY(float worldY) const;
	float TileLeft(int tileX) const;
	float TileRight(int tileX) const;
	float TileTop(int tileY) const;
	float TileBottom(int tileY) const;

	IRenderer* m_renderer = nullptr;
	ITimer* m_timer = nullptr;
	IWindow* m_window = nullptr;
	IInput* m_input = nullptr;
	std::vector<BlockTile> m_blocks;
	std::vector<BlockBreakState> m_blockBreaks;
	PlayerState m_player;
	int m_blockWidth = 64;
	int m_blockHeight = 24;
	int m_surfaceRow = 10;
	float m_tileSize = 32.0f;
	float m_worldOriginX = -1024.0f;
	float m_worldOriginY = 240.0f;
	float m_cameraX = 0.0f;
	float m_playerCollisionWidth = 30.0f;
	float m_playerCollisionHeight = 64.0f;
	float m_playerDrawSize = 160.0f;
	float m_playerSpriteYOffset = 39.0f;
};
