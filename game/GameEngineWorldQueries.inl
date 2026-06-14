// Included by GameEngine.cpp. Shares its private class declarations and file-local helpers.

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
	if (m_world.surfaceHeights.empty())
		return TileTop(m_surfaceRow);

	int tileX = WorldToTileX(worldX);
	if (tileX < 0)
		tileX = 0;
	else if (tileX >= static_cast<int>(m_world.surfaceHeights.size()))
		tileX = static_cast<int>(m_world.surfaceHeights.size()) - 1;

	return TileTop(m_world.surfaceHeights[tileX]);
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

