#include "pch.h"
#include "GameEngine.h"
#include <cmath>

namespace
{
	constexpr int KeyA = 0x41;
	constexpr int KeyD = 0x44;
	constexpr int KeyS = 0x53;
	constexpr int KeyW = 0x57;
	constexpr float BlockBreakDuration = 0.85f;
	constexpr float BlockBreakRecoverDelay = 0.25f;
	constexpr float BlockBreakRecoverDuration = 0.55f;

	const WCHAR* WalkTextures[] =
	{
		L"assets\\char\\walk1.0.png",
		L"assets\\char\\walk1.1.png",
		L"assets\\char\\walk1.2.png",
		L"assets\\char\\walk1.3.png",
	};

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
}

void GameEngine::Start(const char* szTitle, float x, float y, float width, float height, HINSTANCE hInstance)
{
	CreateRenderer(&m_renderer);

	CreateWindowInstance(&m_window);
	m_window->Initialize(szTitle, x, y, width, height, hInstance, this);

	m_renderer->Initialize((UINT)m_window->GetWidth(), (UINT)m_window->GetHeight(), *m_window->GetHWND());

	CreateTimer(&m_timer);
	m_timer->Initialize();

	CreateInput(&m_input);
	m_input->Initialize();
	m_input->AddUser(this);

	m_renderer->LoadTexture(L"assets\\texture\\block_atlas.png");
	m_renderer->LoadTexture(L"assets\\char\\stand.png");
	for (const WCHAR* walkTexture : WalkTextures)
		m_renderer->LoadTexture(walkTexture);
	m_renderer->LoadTexture(L"assets\\char\\jump.0.png");

	InitializeWorld();

	m_timer->Reset();
	m_window->RunMessageLoop();
}

void GameEngine::Update()
{
	float deltaTime = m_timer->GetElapsedSeconds();
	m_timer->Reset();
	if (deltaTime > 0.05f)
		deltaTime = 0.05f;

	m_input->Update();
	UpdateBlockBreaking(deltaTime);
	UpdatePlayer(deltaTime);

	m_renderer->BeginFrame();
	DrawWorld();
	m_renderer->EndFrame();
}

void GameEngine::InitializeWorld()
{
	m_blocks.assign(m_blockWidth * m_blockHeight, BlockTile{});
	m_blockBreaks.assign(m_blockWidth * m_blockHeight, BlockBreakState{});

	for (int y = 0; y < m_blockHeight; ++y)
	{
		for (int x = 0; x < m_blockWidth; ++x)
		{
			BlockTile& tile = m_blocks[y * m_blockWidth + x];
			if (y < m_surfaceRow)
			{
				tile.visible = 0;
				continue;
			}

			tile.visible = 1;
			if (y == m_surfaceRow)
				tile.tileIndex = 0;
			else if (y < m_surfaceRow + 4)
				tile.tileIndex = 1;
			else
				tile.tileIndex = 2;

			if (y > m_surfaceRow + 3 && ((x * 11 + y * 7) % 19 == 0))
				tile.tileIndex = 3;
		}
	}

	const float groundTop = TileTop(m_surfaceRow);
	m_player.x = -220.0f;
	m_player.y = groundTop + (m_playerCollisionHeight * 0.5f) + 0.02f;
	m_player.velocityY = 0.0f;
	m_player.animationTime = 0.0f;
	m_player.facing = 1;
	m_player.onGround = true;
	m_cameraX = m_player.x;
}

void GameEngine::UpdatePlayer(float deltaTime)
{
	const float moveSpeed = 220.0f;
	const float jumpSpeed = 470.0f;
	const float gravity = -1150.0f;
	const float maxFallSpeed = -780.0f;
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

	if (m_input->IsDown(KeyW, this) && m_player.onGround)
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
	m_cameraX += (m_player.x - m_cameraX) * (cameraFollow > 1.0f ? 1.0f : cameraFollow);
}

void GameEngine::UpdateBlockBreaking(float deltaTime)
{
	int miningBlockIndex = -1;
	if (m_input != nullptr && IsKeyHeld(VK_LBUTTON))
	{
		int tileX = 0;
		int tileY = 0;
		if (GetHoveredBlockTile(tileX, tileY))
			miningBlockIndex = tileY * m_blockWidth + tileX;
	}

	for (size_t i = 0; i < m_blockBreaks.size(); ++i)
	{
		BlockBreakState& breakState = m_blockBreaks[i];

		if (m_blocks[i].visible == 0)
		{
			breakState.active = 0;
			breakState.progress = 0.0f;
			breakState.idleTime = 0.0f;
			continue;
		}

		if (static_cast<int>(i) == miningBlockIndex)
		{
			breakState.active = 1;
			breakState.idleTime = 0.0f;
			breakState.progress += deltaTime / BlockBreakDuration;
			if (breakState.progress >= 1.0f)
			{
				m_blocks[i].visible = 0;
				breakState.active = 0;
				breakState.progress = 0.0f;
				breakState.idleTime = 0.0f;
			}

			continue;
		}

		if (!breakState.active)
			continue;

		breakState.idleTime += deltaTime;
		if (breakState.idleTime < BlockBreakRecoverDelay)
			continue;

		breakState.progress -= deltaTime / BlockBreakRecoverDuration;
		if (breakState.progress <= 0.0f)
		{
			breakState.active = 0;
			breakState.progress = 0.0f;
			breakState.idleTime = 0.0f;
		}
	}
}

void GameEngine::DrawWorld()
{
	BlockGridDesc gridDesc;
	gridDesc.textureFile = L"assets\\texture\\block_atlas.png";
	gridDesc.tiles = m_blocks.data();
	gridDesc.width = m_blockWidth;
	gridDesc.height = m_blockHeight;
	gridDesc.atlasColumns = 4;
	gridDesc.atlasRows = 1;
	gridDesc.tileSize = m_tileSize;
	gridDesc.originX = m_worldOriginX - m_cameraX;
	gridDesc.originY = m_worldOriginY;

	m_renderer->DrawBlockGrid(gridDesc);
	DrawBlockCracks();
	DrawHoveredBlockOutline();

	const bool wantsLeft = IsKeyHeld(KeyA);
	const bool wantsRight = IsKeyHeld(KeyD);
	const bool isMoving = wantsLeft != wantsRight;

	SpriteDesc playerDesc;
	playerDesc.positionX = m_player.x - m_cameraX;
	playerDesc.positionY = m_player.y + m_playerSpriteYOffset;
	playerDesc.width = m_playerDrawSize;
	playerDesc.height = m_playerDrawSize;
	playerDesc.flipX = m_player.facing < 0 ? 1 : 0;
	playerDesc.depth = -1.0f;

	if (!m_player.onGround)
	{
		playerDesc.textureFile = L"assets\\char\\jump.0.png";
	}
	else if (isMoving)
	{
		const int frame = static_cast<int>(m_player.animationTime / 0.11f) % 4;
		playerDesc.textureFile = WalkTextures[frame];
	}
	else
	{
		playerDesc.textureFile = L"assets\\char\\stand.png";
		playerDesc.atlasColumns = 4;
		playerDesc.atlasRows = 1;
		playerDesc.tileIndex = static_cast<int>(m_player.animationTime / 0.24f) % 4;
	}

	m_renderer->DrawSprite(playerDesc);
}

void GameEngine::DrawBlockCracks()
{
	for (int y = 0; y < m_blockHeight; ++y)
	{
		for (int x = 0; x < m_blockWidth; ++x)
		{
			const int blockIndex = y * m_blockWidth + x;
			const BlockBreakState& breakState = m_blockBreaks[blockIndex];
			if (!breakState.active || m_blocks[blockIndex].visible == 0)
				continue;

			const float progress = Clamp01(breakState.progress);
			const float centerX = m_worldOriginX + x * m_tileSize - m_cameraX;
			const float centerY = m_worldOriginY - y * m_tileSize;
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

void GameEngine::DrawHoveredBlockOutline()
{
	int tileX = 0;
	int tileY = 0;
	if (!GetHoveredBlockTile(tileX, tileY))
		return;

	RectOutlineDesc outlineDesc;
	outlineDesc.positionX = m_worldOriginX + tileX * m_tileSize - m_cameraX;
	outlineDesc.positionY = m_worldOriginY - tileY * m_tileSize;
	outlineDesc.width = m_tileSize;
	outlineDesc.height = m_tileSize;
	outlineDesc.thickness = 2.0f;
	outlineDesc.depth = -0.5f;

	m_renderer->DrawRectOutline(outlineDesc);
}

bool GameEngine::GetHoveredBlockTile(int& tileX, int& tileY) const
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

	constexpr float renderCameraDistance = 500.0f;
	constexpr float renderFovYDegrees = 60.0f;
	const float viewHalfHeight = std::tan((renderFovYDegrees * Deg2Rad) * 0.5f) * renderCameraDistance;
	const float viewHalfWidth = viewHalfHeight * (windowWidth / windowHeight);
	const float normalizedX = (static_cast<float>(cursorPosition.x) / windowWidth) * 2.0f - 1.0f;
	const float normalizedY = 1.0f - (static_cast<float>(cursorPosition.y) / windowHeight) * 2.0f;
	const float worldX = normalizedX * viewHalfWidth + m_cameraX;
	const float worldY = normalizedY * viewHalfHeight;

	tileX = WorldToTileX(worldX);
	tileY = WorldToTileY(worldY);
	if (tileX < 0 || tileX >= m_blockWidth || tileY < 0 || tileY >= m_blockHeight)
		return false;

	return m_blocks[tileY * m_blockWidth + tileX].visible != 0;
}

collib::AABB GameEngine::GetPlayerAABB(float playerX, float playerY) const
{
	return collib::MakeAABB(playerX, playerY, m_playerCollisionWidth, m_playerCollisionHeight);
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

bool GameEngine::IsSolidTile(int tileX, int tileY) const
{
	if (tileY < 0)
		return false;
	if (tileX < 0 || tileX >= m_blockWidth || tileY >= m_blockHeight)
		return true;

	return m_blocks[tileY * m_blockWidth + tileX].visible != 0;
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
}
