// Included by GameEngine.cpp. Shares its private class declarations and file-local helpers.

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
	SetStatusText("지도 공개됨", 1.6f);
}

void GameEngine::UpdateInventoryInput()
{
	if (!m_acceptInput)
		return;

	constexpr int HotkeySlotCount = 10;
	for (int slot = 0; slot < HotkeySlotCount; ++slot)
	{
		const int keyCode = slot < 9 ? 0x31 + slot : 0x30;
		if (IsKeyDown(keyCode))
			m_selectedInventorySlot = slot;
	}

	float cursorX = 0.0f;
	float cursorY = 0.0f;
	if (GetCursorViewPosition(cursorX, cursorY))
	{
		const int hoveredSlot = GetInventorySlotAt(cursorX, cursorY);
		if (hoveredSlot >= 0)
		{
			if (IsKeyHeld(VK_LBUTTON))
				m_uiConsumesLeftMouse = true;
			if (IsKeyDown(VK_LBUTTON))
				m_selectedInventorySlot = hoveredSlot;
		}
	}
}

void GameEngine::UpdateDebugLogInput()
{
	float cursorX = 0.0f;
	float cursorY = 0.0f;
	if (!GetCursorViewPosition(cursorX, cursorY))
		return;

	const UiRect panel = GetLogPanelRect();
	const float panelCenterX = panel.left + panel.width * 0.5f;
	const float panelCenterY = panel.top - panel.height * 0.5f;
	if (!IsPointInsideRect(cursorX, cursorY, panelCenterX, panelCenterY, panel.width, panel.height))
		return;

	if (IsKeyHeld(VK_LBUTTON))
		m_uiConsumesLeftMouse = true;
	if (!IsKeyDown(VK_LBUTTON))
		return;

	const float buttonHeight = 20.0f;
	const float buttonY = panel.top - 53.0f;
	const float perfX = panel.left + 46.0f;
	const float capX = panel.left + 122.0f;
	const float revealX = panel.left + 205.0f;
	if (IsPointInsideRect(cursorX, cursorY, perfX, buttonY, 68.0f, buttonHeight))
	{
		m_showRenderStats = !m_showRenderStats;
		return;
	}
	if (IsPointInsideRect(cursorX, cursorY, capX, buttonY, 68.0f, buttonHeight))
	{
		ToggleFrameLimiter();
		return;
	}
	if (IsPointInsideRect(cursorX, cursorY, revealX, buttonY, 78.0f, buttonHeight))
	{
		RevealAllMap();
		return;
	}
}

void GameEngine::ClampCraftingScrollOffset()
{
	m_craftingScrollOffset = std::clamp(m_craftingScrollOffset, 0, GetMaxCraftingScrollOffset());
}

int GameEngine::GetCraftingRecipeCount() const
{
	return IsCraftingTableNearby() ? 3 : 1;
}

int GameEngine::GetVisibleCraftingRecipeRows() const
{
	const CraftingPanelLayout layout = GetCraftingPanelLayout();
	const float rowStride = layout.rowHeight;
	if (rowStride <= 0.0f)
		return 1;

	return (std::max)(1, static_cast<int>(layout.height / rowStride));
}

int GameEngine::GetMaxCraftingScrollOffset() const
{
	return (std::max)(0, GetCraftingRecipeCount() - GetVisibleCraftingRecipeRows());
}

bool GameEngine::CraftSword()
{
	if (!CanCraftSword())
	{
		SetStatusText("재료 부족: 나무 2 돌 3", 1.5f);
		return false;
	}

	m_inventoryCounts[SlotWood] -= 2;
	m_inventoryCounts[SlotStone] -= 3;
	++m_inventoryCounts[SlotSword];
	m_selectedInventorySlot = SlotSword;
	SetStatusText("검 제작됨", 1.8f);
	return true;
}

bool GameEngine::CraftAxe()
{
	if (!CanCraftAxe())
	{
		SetStatusText("재료 부족: 나무 3 돌 2", 1.5f);
		return false;
	}

	m_inventoryCounts[SlotWood] -= 3;
	m_inventoryCounts[SlotStone] -= 2;
	++m_inventoryCounts[SlotAxe];
	m_selectedInventorySlot = SlotAxe;
	SetStatusText("도끼 제작됨", 1.8f);
	return true;
}

bool GameEngine::CraftTable()
{
	if (!CanCraftTable())
	{
		SetStatusText("재료 부족: 나무 4", 1.5f);
		return false;
	}

	m_inventoryCounts[SlotWood] -= 4;
	++m_inventoryCounts[SlotCraftingTable];
	m_selectedInventorySlot = SlotCraftingTable;
	SetStatusText("작업대 제작됨", 1.8f);
	return true;
}

bool GameEngine::CraftRecipe(int recipeIndex)
{
	ClampCraftingScrollOffset();
	if (recipeIndex < 0 || recipeIndex >= GetCraftingRecipeCount())
		return false;

	if (recipeIndex == 0)
		return CraftTable();
	if (recipeIndex == 1 && IsCraftingTableNearby())
		return CraftSword();
	if (recipeIndex == 2 && IsCraftingTableNearby())
		return CraftAxe();

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

	if (!m_acceptInput || m_input == nullptr)
		return;

	if (m_window != nullptr)
	{
		const int wheelDelta = m_window->ConsumeMouseWheelDelta();
		if (wheelDelta != 0)
		{
			if (IsCursorOverCraftingPanel())
			{
				m_craftingWheelRemainder += wheelDelta;
				const int wheelSteps = m_craftingWheelRemainder / WHEEL_DELTA;
				if (wheelSteps != 0)
				{
					m_craftingWheelRemainder -= wheelSteps * WHEEL_DELTA;
					m_craftingScrollOffset -= wheelSteps;
					ClampCraftingScrollOffset();
				}
			}
			else
			{
				m_inventoryWheelRemainder += wheelDelta;
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
		}
	}

	ClampCraftingScrollOffset();

	if (IsCursorOverCraftingPanel() && IsKeyHeld(VK_LBUTTON))
		m_uiConsumesLeftMouse = true;

	if (!IsKeyDown(VK_LBUTTON))
		return;

	float cursorX = 0.0f;
	float cursorY = 0.0f;
	if (!GetCursorViewPosition(cursorX, cursorY))
		return;

	const int recipe = GetCraftingRecipeAt(cursorX, cursorY);
	if (recipe < 0)
		return;

	m_uiConsumesLeftMouse = true;
	CraftRecipe(recipe);
}

void GameEngine::TryPlaceSelectedBlock(float deltaTime)
{
	if (m_blockPlaceRepeatTimer > 0.0f)
		m_blockPlaceRepeatTimer = (std::max)(0.0f, m_blockPlaceRepeatTimer - deltaTime);

	if (!IsKeyHeld(VK_RBUTTON))
	{
		m_blockPlaceRepeatTimer = 0.0f;
		m_lastPlacedTileX = -1;
		m_lastPlacedTileY = -1;
		return;
	}

	if (IsCursorOverCraftingPanel())
		return;

	int tileX = 0;
	int tileY = 0;
	if (!GetHoveredTile(tileX, tileY) || !CanPlaceSelectedBlockAt(tileX, tileY))
		return;

	const bool newTile = tileX != m_lastPlacedTileX || tileY != m_lastPlacedTileY;
	if (!IsKeyDown(VK_RBUTTON) && !newTile && m_blockPlaceRepeatTimer > 0.0f)
		return;

	const int blockIndex = tileY * m_blockWidth + tileX;
	m_blocks[blockIndex].visible = 1;
	m_blocks[blockIndex].tileIndex = GetPlacementTileIndexForSlot(m_selectedInventorySlot);
	m_blockBreaks[blockIndex] = BlockBreakState{};
	MarkBlockChunkDirty(tileX, tileY);
	--m_inventoryCounts[m_selectedInventorySlot];
	PublishLocalTileEdit(tileX, tileY);
	m_blockPlaceRepeatTimer = BlockPlaceRepeatInterval;
	m_lastPlacedTileX = tileX;
	m_lastPlacedTileY = tileY;
}

void GameEngine::UpdatePlayer(float deltaTime)
{
	const float moveSpeed = m_tileSize * GetPlayerMoveSpeedTiles();
	const float jumpSpeed = m_tileSize * GetPlayerJumpSpeedTiles();
	const float gravity = -m_tileSize * 36.0f;
	const float maxFallSpeed = -m_tileSize * 26.0f;
	const float collisionInset = 0.5f;
	const float skin = 0.02f;

	if (m_playerKnockbackCooldownTimer > 0.0f)
		m_playerKnockbackCooldownTimer = (std::max)(0.0f, m_playerKnockbackCooldownTimer - deltaTime);
	if (m_playerKnockbackTimer > 0.0f)
	{
		m_playerKnockbackTimer = (std::max)(0.0f, m_playerKnockbackTimer - deltaTime);
		if (m_playerKnockbackTimer <= 0.0f)
			m_player.velocityX = 0.0f;
	}

	float move = 0.0f;
	if (IsKeyHeld(KeyA))
		move -= 1.0f;
	if (IsKeyHeld(KeyD))
		move += 1.0f;

	if (move < 0.0f)
		m_player.facing = -1;
	else if (move > 0.0f)
		m_player.facing = 1;

	const bool knockbackActive = m_playerKnockbackTimer > 0.0f && std::fabs(m_player.velocityX) > 0.01f;
	const float controlScale = knockbackActive ? 0.35f : 1.0f;
	const float deltaX = (move * moveSpeed * controlScale + (knockbackActive ? m_player.velocityX : 0.0f)) * deltaTime;
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
	if (knockbackActive)
	{
		if (IsAABBBlocked(GetPlayerAABB(nextX, m_player.y), collisionInset) && deltaX * m_player.velocityX > 0.0f)
		{
			m_player.velocityX = 0.0f;
			m_playerKnockbackTimer = 0.0f;
		}
		else
		{
			const float deceleration = m_tileSize * 34.0f * deltaTime;
			if (m_player.velocityX > 0.0f)
				m_player.velocityX = (std::max)(0.0f, m_player.velocityX - deceleration);
			else
				m_player.velocityX = (std::min)(0.0f, m_player.velocityX + deceleration);
		}
	}

	if (IsKeyDown(KeyJump) && m_player.onGround)
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
		m_playerInvulnerableTimer = (std::max)(0.0f, m_playerInvulnerableTimer - deltaTime);
	if (m_playerHurtFlashTimer > 0.0f)
		m_playerHurtFlashTimer = (std::max)(0.0f, m_playerHurtFlashTimer - deltaTime);

	if (m_playerHealth <= 0)
	{
		m_player.x = m_playerSpawnX;
		m_player.y = m_playerSpawnY;
		m_player.velocityX = 0.0f;
		m_player.velocityY = 0.0f;
		m_player.onGround = true;
		m_cameraX = m_player.x;
		m_cameraY = m_player.y;
		m_playerHealth = GetPlayerMaxHealth();
		m_playerInvulnerableTimer = 1.5f;
		m_playerHurtFlashTimer = 0.0f;
		m_playerKnockbackTimer = 0.0f;
		m_playerKnockbackCooldownTimer = 0.0f;
		SetStatusText("부활", 1.8f);
	}

	TryPlayerAttack();
}

void GameEngine::TryPlayerAttack()
{
	if (m_uiConsumesLeftMouse || !IsKeyHeld(VK_LBUTTON))
		return;

	if (!ShouldLeftClickAttack())
		return;

	if (m_attackCooldown > 0.0f)
		return;

	m_attackCooldown = 0.34f;
	m_attackTimer = PlayerAttackDuration;
	const bool swordEquipped = m_selectedInventorySlot == SlotSword && m_inventoryCounts[SlotSword] > 0;
	const bool axeEquipped = m_selectedInventorySlot == SlotAxe && m_inventoryCounts[SlotAxe] > 0;
	const int attackDamage = GetPlayerAttackDamage();

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
		const float knockbackDistance = m_player.facing * (swordEquipped ? 22.0f : (axeEquipped ? 11.0f : 9.0f));
		const int stepCount = (std::max)(1, static_cast<int>(std::ceil(std::fabs(knockbackDistance) / 3.0f)));
		const float stepX = knockbackDistance / static_cast<float>(stepCount);
		for (int step = 0; step < stepCount; ++step)
		{
			const float nextX = monster.x + stepX;
			if (IsAABBBlocked(GetMonsterAABB(nextX, monster.y), 0.5f))
				break;

			monster.x = nextX;
		}
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
		SetStatusText("명중", 0.5f);
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
		const float chaseLeashDistance = monster.underground ? m_tileSize * 26.0f : m_tileSize * 18.0f;
		const bool tooFarFromHome = std::fabs(monster.x - monster.homeX) > chaseLeashDistance;
		const bool chasingPlayer = !tooFarFromHome && playerDistanceX < awarenessX && playerDistanceY < awarenessY;
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
				if (tooFarFromHome)
					monster.aiTimer = (std::min)(monster.aiTimer, 0.25f);
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
			const int incomingDamage = monster.underground ? 11 : 8;
			const int damageTaken = (std::max)(1, incomingDamage - GetPlayerDefense());
			m_playerHealth -= damageTaken;
			m_playerInvulnerableTimer = 0.75f;
			m_playerHurtFlashTimer = PlayerHurtFlashDuration;
			if (m_playerKnockbackCooldownTimer <= 0.0f)
			{
				int knockbackDirection = playerDeltaX >= 0.0f ? 1 : -1;
				if (playerDistanceX < 1.0f)
					knockbackDirection = monster.facing != 0 ? monster.facing : knockbackDirection;

				const float knockbackSpeed = m_tileSize * (monster.underground ? 12.2f : 10.8f);
				const float liftSpeed = m_tileSize * (m_player.onGround ? 6.2f : 4.4f);
				m_player.velocityX = knockbackDirection * knockbackSpeed;
				m_player.velocityY = (std::max)(m_player.velocityY, liftSpeed);
				m_player.onGround = false;
				m_playerKnockbackTimer = PlayerKnockbackDuration;
				m_playerKnockbackCooldownTimer = PlayerKnockbackCooldownDuration;
			}
			monster.attackCooldown = monster.underground ? 0.85f : 1.05f;
			if (monster.contactDirection == 0)
				monster.contactDirection = monster.facing;
			monster.contactTimer = (std::max)(monster.contactTimer, 0.28f);
			SetStatusText("몬스터 명중", 0.8f);
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
	const float maxMagnetSpeed = m_tileSize * 38.0f;
	const float skin = 0.02f;

	for (DroppedItemState& item : m_droppedItems)
	{
		if (!item.alive)
			continue;

		if (item.pickupDelay > 0.0f)
			item.pickupDelay -= deltaTime;

		const float deltaToLocalPlayerX = m_player.x - item.x;
		const float deltaToLocalPlayerY = m_player.y - item.y;
		const float localDistanceSq = deltaToLocalPlayerX * deltaToLocalPlayerX + deltaToLocalPlayerY * deltaToLocalPlayerY;
		if (CanLocalPlayerPickupItem(item) && item.pickupDelay <= 0.0f && localDistanceSq <= pickupRadius * pickupRadius)
		{
			AddBlockToInventory(item.tileIndex, item.amount);
			EnsureDroppedItemNetworkId(item);
			item.alive = false;
			PublishDroppedItemState(item);
			continue;
		}

		float targetX = 0.0f;
		float targetY = 0.0f;
		const bool hasPickupTarget = TryGetDroppedItemPickupPosition(item, targetX, targetY);
		const float deltaToTargetX = targetX - item.x;
		const float deltaToTargetY = targetY - item.y;
		const float targetDistanceSq = deltaToTargetX * deltaToTargetX + deltaToTargetY * deltaToTargetY;
		if (item.pickupDelay <= 0.0f && hasPickupTarget && targetDistanceSq <= magnetRadius * magnetRadius)
		{
			const float distance = std::sqrt((std::max)(targetDistanceSq, 0.001f));
			const float pullStrength = 1.0f - Clamp01(distance / magnetRadius);
			const float targetSpeed = m_tileSize * (18.0f + pullStrength * 32.0f);
			const float targetVelocityX = (deltaToTargetX / distance) * targetSpeed;
			const float targetVelocityY = (deltaToTargetY / distance) * targetSpeed;
			const float steer = Clamp01(deltaTime * (10.0f + pullStrength * 18.0f));
			item.velocityX += (targetVelocityX - item.velocityX) * steer;
			item.velocityY += (targetVelocityY - item.velocityY) * steer;

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

void GameEngine::UpdateLeafParticles(float deltaTime)
{
	const float gravity = -m_tileSize * 18.0f;
	for (LeafParticleState& particle : m_leafParticles)
	{
		if (!particle.alive)
			continue;

		particle.age += deltaTime;
		if (particle.age >= particle.lifetime)
		{
			particle.alive = false;
			continue;
		}

		particle.velocityY += gravity * deltaTime;
		particle.velocityX *= (std::max)(0.0f, 1.0f - deltaTime * 0.85f);
		particle.x += particle.velocityX * deltaTime;
		particle.y += particle.velocityY * deltaTime;
		particle.rotation += particle.angularVelocity * deltaTime;
	}
}

void GameEngine::UpdateBlockBreaking(float deltaTime)
{
	auto stopPublishedBreak = [this]()
	{
		if (m_networkBreakingBlockIndex < 0)
			return;

		const int tileX = m_networkBreakingBlockIndex % m_blockWidth;
		const int tileY = m_networkBreakingBlockIndex / m_blockWidth;
		if (IsTileInBounds(tileX, tileY))
			PublishBlockBreakState(tileX, tileY, 0.0f, false);
		m_networkBreakingBlockIndex = -1;
	};

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
	{
		stopPublishedBreak();
		return;
	}

	if (miningBlockIndex < 0 || miningBlockIndex >= static_cast<int>(m_blockBreaks.size()))
	{
		stopPublishedBreak();
		return;
	}

	if (m_blocks[miningBlockIndex].visible == 0)
	{
		m_blockBreaks[miningBlockIndex] = BlockBreakState();
		stopPublishedBreak();
		return;
	}

	if (m_networkBreakingBlockIndex >= 0 && m_networkBreakingBlockIndex != miningBlockIndex)
		stopPublishedBreak();
	m_networkBreakingBlockIndex = miningBlockIndex;

	BlockBreakState& breakState = m_blockBreaks[miningBlockIndex];
	breakState.active = 1;
	breakState.idleTime = 0.0f;
	breakState.progress += deltaTime / GetBlockBreakDuration(m_blocks[miningBlockIndex].tileIndex);
	const int tileX = miningBlockIndex % m_blockWidth;
	const int tileY = miningBlockIndex / m_blockWidth;
	PublishBlockBreakState(tileX, tileY, breakState.progress, true);
	if (breakState.progress < 1.0f)
		return;

	TryHarvestTreeAt(tileX, tileY);
	if (m_blocks[miningBlockIndex].visible != 0)
	{
		const float dropX = m_worldOriginX + tileX * m_tileSize;
		const float dropY = m_worldOriginY - tileY * m_tileSize;
		const unsigned short tileIndex = m_blocks[miningBlockIndex].tileIndex;
		if (tileIndex != BlockLeaves)
			SpawnDroppedItem(dropX, dropY, GetDroppedItemTileIndex(tileIndex), 1, tileIndex != BlockWood && tileIndex != BlockPlacedWood);
		else
			SpawnLeafBreakEffect(tileX, tileY);
		m_blocks[miningBlockIndex].visible = 0;
		MarkBlockChunkDirty(tileX, tileY);
		PublishLocalTileEdit(tileX, tileY);
	}

	breakState.active = 0;
	breakState.progress = 0.0f;
	breakState.idleTime = 0.0f;
	PublishBlockBreakState(tileX, tileY, 0.0f, false);
	m_networkBreakingBlockIndex = -1;
}

bool GameEngine::GetHoveredTile(int& tileX, int& tileY) const
{
	float viewX = 0.0f;
	float viewY = 0.0f;
	if (!GetCursorGameViewPosition(viewX, viewY))
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

bool GameEngine::IsWindowFocused() const
{
	if (m_window == nullptr)
		return false;

	HWND* hwnd = m_window->GetHWND();
	if (hwnd == nullptr || *hwnd == nullptr)
		return false;

	const HWND foregroundWindow = GetForegroundWindow();
	return foregroundWindow == *hwnd || IsChild(*hwnd, foregroundWindow);
}

bool GameEngine::IsKeyDown(int keyCode) const
{
	return m_acceptInput && m_input != nullptr && m_input->IsDown(keyCode, const_cast<GameEngine*>(this));
}

bool GameEngine::IsKeyHeld(int keyCode)
{
	return m_acceptInput && m_input != nullptr && (m_input->IsDown(keyCode, this) || m_input->IsPressed(keyCode, this));
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

bool GameEngine::CanCraftTable() const
{
	return m_inventoryCounts[SlotWood] >= 4;
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

	if (IsTopSurfaceOnlyItem(m_selectedInventorySlot))
	{
		if (!CanPlaceBlockAt(tileX, tileY))
			return false;

		return tileY < m_blockHeight - 1 && IsSolidTile(tileX, tileY + 1);
	}

	return CanPlaceBlockAt(tileX, tileY);
}

bool GameEngine::ShouldLeftClickAttack() const
{
	int tileX = 0;
	int tileY = 0;
	if (!GetHoveredTile(tileX, tileY))
		return false;

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

	std::vector<unsigned char> treeVisited(m_blocks.size(), 0);
	std::vector<int> treeQueue;
	std::vector<int> treeWoodIndices;
	treeQueue.reserve(96);
	treeWoodIndices.reserve(96);

	auto tryAddTreeWood = [&](int nextX, int nextY)
	{
		if (!IsTileInBounds(nextX, nextY))
			return;

		const int nextIndex = nextY * m_blockWidth + nextX;
		if (treeVisited[nextIndex] != 0)
			return;

		const BlockTile& tile = m_blocks[nextIndex];
		if (tile.visible == 0 || tile.tileIndex != BlockWood)
			return;

		treeVisited[nextIndex] = 1;
		treeQueue.push_back(nextIndex);
	};

	tryAddTreeWood(startX, startY);
	for (size_t head = 0; head < treeQueue.size(); ++head)
	{
		const int currentIndex = treeQueue[head];
		treeWoodIndices.push_back(currentIndex);
		const int currentX = currentIndex % m_blockWidth;
		const int currentY = currentIndex / m_blockWidth;
		tryAddTreeWood(currentX - 1, currentY);
		tryAddTreeWood(currentX + 1, currentY);
		tryAddTreeWood(currentX, currentY - 1);
		tryAddTreeWood(currentX, currentY + 1);
	}

	std::vector<unsigned char> fallingVisited(m_blocks.size(), 0);
	std::vector<int> fallingQueue;
	std::vector<int> fallingWoodIndices;
	fallingQueue.reserve(treeWoodIndices.size());
	fallingWoodIndices.reserve(treeWoodIndices.size());

	auto tryAddFallingWood = [&](int nextX, int nextY)
	{
		if (!IsTileInBounds(nextX, nextY))
			return;
		if (nextY > startY)
			return;

		const int nextIndex = nextY * m_blockWidth + nextX;
		if (fallingVisited[nextIndex] != 0)
			return;

		const BlockTile& tile = m_blocks[nextIndex];
		if (tile.visible == 0 || tile.tileIndex != BlockWood)
			return;

		fallingVisited[nextIndex] = 1;
		fallingQueue.push_back(nextIndex);
	};

	tryAddFallingWood(startX, startY);
	for (size_t head = 0; head < fallingQueue.size(); ++head)
	{
		const int currentIndex = fallingQueue[head];
		if (currentIndex != blockIndex)
			fallingWoodIndices.push_back(currentIndex);

		const int currentX = currentIndex % m_blockWidth;
		const int currentY = currentIndex / m_blockWidth;
		tryAddFallingWood(currentX - 1, currentY);
		tryAddFallingWood(currentX + 1, currentY);
		tryAddFallingWood(currentX, currentY - 1);
		tryAddFallingWood(currentX, currentY + 1);
	}

	std::vector<int> leafIndices;
	leafIndices.reserve(128);
	std::vector<unsigned char> leafVisited(m_blocks.size(), 0);
	constexpr int LeafSearchRadius = 6;

	auto nearestTreeWoodDistance = [&](int leafX, int leafY)
	{
		int bestDistance = 9999;
		for (int woodIndex : treeWoodIndices)
		{
			const int woodX = woodIndex % m_blockWidth;
			const int woodY = woodIndex / m_blockWidth;
			const int distance = std::abs(leafX - woodX) + std::abs(leafY - woodY);
			if (distance < bestDistance)
				bestDistance = distance;
		}

		return bestDistance;
	};

	auto isClaimedByOtherTree = [&](int leafX, int leafY, int treeDistance)
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
				if (tile.visible == 0 || tile.tileIndex != BlockWood || treeVisited[woodIndex] != 0)
					continue;

				const int distance = std::abs(offsetX) + std::abs(offsetY);
				if (distance + 1 < treeDistance)
					return true;
			}
		}

		return false;
	};

	auto tryAddLeaf = [&](int nextX, int nextY)
	{
		if (!IsTileInBounds(nextX, nextY))
			return;

		const int nextIndex = nextY * m_blockWidth + nextX;
		if (leafVisited[nextIndex] != 0)
			return;

		const BlockTile& tile = m_blocks[nextIndex];
		if (tile.visible == 0 || tile.tileIndex != BlockLeaves)
			return;

		const int treeDistance = nearestTreeWoodDistance(nextX, nextY);
		if (treeDistance > LeafSearchRadius + 2)
			return;
		if (isClaimedByOtherTree(nextX, nextY, treeDistance))
			return;

		leafVisited[nextIndex] = 1;
		leafIndices.push_back(nextIndex);
	};

	for (int woodIndex : treeWoodIndices)
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
		PublishLocalTileEdit(woodIndex % m_blockWidth, woodIndex / m_blockWidth);
	}

	for (int leafIndex : leafIndices)
	{
		SpawnLeafBreakEffect(leafIndex % m_blockWidth, leafIndex / m_blockWidth);
		m_blocks[leafIndex].visible = 0;
		if (leafIndex >= 0 && leafIndex < static_cast<int>(m_blockBreaks.size()))
			m_blockBreaks[leafIndex] = BlockBreakState();
		MarkBlockIndexDirty(leafIndex);
		PublishLocalTileEdit(leafIndex % m_blockWidth, leafIndex / m_blockWidth);
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
	if (tileIndex == BlockWood || tileIndex == BlockPlacedWood || tileIndex == BlockLeaves)
		return BlockBreakDuration / GetSelectedChopSpeedMultiplier();

	return BlockBreakDuration;
}

int GameEngine::GetPlayerMaxHealth() const
{
	return PlayerBaseMaxHealth;
}

int GameEngine::GetPlayerDefense() const
{
	int defense = PlayerBaseDefense;
	if (IsInventoryWeaponSlot(m_selectedInventorySlot) && m_inventoryCounts[m_selectedInventorySlot] > 0)
		defense += GetEquipmentStatsForSlot(m_selectedInventorySlot).defenseBonus;

	return defense;
}

int GameEngine::GetPlayerAttackDamage() const
{
	int attack = PlayerBaseAttack;
	if (IsInventoryWeaponSlot(m_selectedInventorySlot) && m_inventoryCounts[m_selectedInventorySlot] > 0)
		attack += GetEquipmentStatsForSlot(m_selectedInventorySlot).attackBonus;

	return attack;
}

float GameEngine::GetPlayerMoveSpeedTiles() const
{
	return PlayerMoveSpeedTiles;
}

float GameEngine::GetPlayerJumpSpeedTiles() const
{
	return PlayerJumpSpeedTiles;
}

float GameEngine::GetSelectedChopSpeedMultiplier() const
{
	if (IsInventoryWeaponSlot(m_selectedInventorySlot) && m_inventoryCounts[m_selectedInventorySlot] > 0)
		return GetEquipmentStatsForSlot(m_selectedInventorySlot).chopSpeedMultiplier;

	return 1.0f;
}

void GameEngine::SpawnLeafBreakEffect(int tileX, int tileY)
{
	if (!IsTileInBounds(tileX, tileY))
		return;

	const unsigned int seed = HashUInt(GetTickCount() ^
		(static_cast<unsigned int>(tileX) * 374761393u) ^
		(static_cast<unsigned int>(tileY) * 668265263u));
	SpawnLeafBreakEffectWithSeed(tileX, tileY, seed);
	PublishLeafBreakEffect(tileX, tileY, seed);
}

void GameEngine::SpawnLeafBreakEffectWithSeed(int tileX, int tileY, unsigned int seed)
{
	if (!IsTileInBounds(tileX, tileY))
		return;

	constexpr int ShardCount = 4;
	constexpr size_t MaxLeafParticles = 420;
	const float centerX = m_worldOriginX + tileX * m_tileSize;
	const float centerY = m_worldOriginY - tileY * m_tileSize;

	auto addParticle = [this](const LeafParticleState& particle)
	{
		for (LeafParticleState& existing : m_leafParticles)
		{
			if (existing.alive)
				continue;

			existing = particle;
			return;
		}

		if (m_leafParticles.size() < MaxLeafParticles)
		{
			m_leafParticles.push_back(particle);
			return;
		}

		LeafParticleState* oldest = nullptr;
		for (LeafParticleState& existing : m_leafParticles)
		{
			if (oldest == nullptr || existing.age > oldest->age)
				oldest = &existing;
		}
		if (oldest != nullptr)
			*oldest = particle;
	};

	for (int i = 0; i < ShardCount; ++i)
	{
		const float randomA = Hash01(tileX + i * 17, tileY - i * 13, seed + 101u);
		const float randomB = Hash01(tileX - i * 11, tileY + i * 19, seed + 211u);
		const float randomC = Hash01(tileX + i * 29, tileY + i * 7, seed + 307u);
		const float direction = randomA < 0.5f ? -1.0f : 1.0f;

		LeafParticleState particle;
		particle.x = centerX + (randomA - 0.5f) * m_tileSize * 0.58f;
		particle.y = centerY + (randomB - 0.5f) * m_tileSize * 0.58f;
		particle.velocityX = direction * m_tileSize * (3.4f + randomB * 6.8f);
		particle.velocityY = m_tileSize * (3.8f + randomC * 7.4f);
		particle.lifetime = 0.46f + randomA * 0.30f;
		particle.rotation = randomB * 6.2831853f;
		particle.angularVelocity = direction * (3.6f + randomC * 7.2f);
		particle.size = m_tileSize * (0.30f + randomB * 0.24f);
		particle.alive = true;
		addParticle(particle);
	}
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
	case BlockPlacedWood:
		tileIndex = BlockWood;
		break;
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
	const int pickupPlayerId = ChooseDroppedItemPickupPlayer(worldX, worldY);
	if (mergeNearby)
	{
		for (DroppedItemState& item : m_droppedItems)
		{
			if (!item.alive || item.tileIndex != tileIndex)
				continue;
			if (m_networkMode != NetworkConfig::Mode::SinglePlayer && !IsNetworkItemOwnedByLocal(item))
				continue;
			const int existingPickupPlayerId = item.pickupPlayerId != 0 ? item.pickupPlayerId : m_localPlayerId;
			if (existingPickupPlayerId != pickupPlayerId)
				continue;

			const float dx = item.x - worldX;
			const float dy = item.y - worldY;
			if (dx * dx + dy * dy > mergeDistance * mergeDistance)
				continue;

			item.amount += amount;
			item.pickupPlayerId = pickupPlayerId;
			item.pickupDelay = (std::max)(item.pickupDelay, 0.18f);
			item.velocityY = (std::max)(item.velocityY, m_tileSize * 4.0f);
			EnsureDroppedItemNetworkId(item);
			PublishDroppedItemState(item);
			return;
		}
	}

	DroppedItemState item;
	item.x = worldX;
	item.y = worldY;
	item.tileIndex = tileIndex;
	item.amount = amount;
	item.pickupPlayerId = pickupPlayerId;
	item.alive = true;
	item.pickupDelay = 0.24f;

	const int hashX = WorldToTileX(worldX);
	const int hashY = WorldToTileY(worldY);
	const float scatter = Hash01(hashX, hashY, GetTickCount()) - 0.5f;
	const float pop = Hash01(hashX + 13, hashY - 7, GetTickCount() + 91u);
	item.velocityX = scatter * m_tileSize * 5.0f;
	item.velocityY = m_tileSize * (4.4f + pop * 2.8f);
	EnsureDroppedItemNetworkId(item);

	for (DroppedItemState& existing : m_droppedItems)
	{
		if (existing.alive)
			continue;

		existing = item;
		PublishDroppedItemState(existing);
		return;
	}

	m_droppedItems.push_back(item);
	PublishDroppedItemState(m_droppedItems.back());
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
