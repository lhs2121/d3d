#pragma once
#include <Windows.h>
#include <commonlib/Interface.h>
#include <inputlib/Interface.h>
#include <collib/Interface.h>
#include <rendererlib/Interface.h>
#include <windowlib/Interface.h>
#include <vector>

class GameEngine : public IEngine
{
public:
	void Start(const char* szTitle, float x, float y, float width, float height, HINSTANCE hInstance);
	void EngineUpdate() override;
	void EngineRelease() override;

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

	void InitializeWorld();
	void UpdatePlayer(float deltaTime);
	void DrawWorld();
	void DrawHoveredBlockOutline();
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

	IRenderer* m_pRenderer = nullptr;
	ITimeObject* m_pTimeObject = nullptr;
	IWindowObject* m_pWindowObject = nullptr;
	IInputObject* m_pInputObject = nullptr;
	std::vector<BlockTile> m_blocks;
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
