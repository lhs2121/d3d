#pragma once

#include <array>
#include <networklib/Interface.h>

class PlayerState
{
public:
	float x = 0.0f;
	float y = 0.0f;
	float velocityX = 0.0f;
	float velocityY = 0.0f;
	float animationTime = 0.0f;
	int facing = 1;
	bool onGround = false;
};

class BlockBreakState
{
public:
	float progress = 0.0f;
	float idleTime = 0.0f;
	unsigned char active = 0;
};

class MonsterState
{
public:
	float x = 0.0f;
	float y = 0.0f;
	float velocityY = 0.0f;
	float hurtTimer = 0.0f;
	float attackCooldown = 0.0f;
	float aiTimer = 0.0f;
	float idleTimer = 0.0f;
	float jumpCooldown = 0.0f;
	float jumpMoveTimer = 0.0f;
	float chaseFacingLockTimer = 0.0f;
	float stuckTimer = 0.0f;
	float animationTime = 0.0f;
	float contactTimer = 0.0f;
	float homeX = 0.0f;
	float lastX = 0.0f;
	int facing = -1;
	int contactDirection = 0;
	int jumpMoveDirection = 0;
	int chaseFacingDirection = 0;
	int health = 40;
	int maxHealth = 40;
	unsigned char biome = 0;
	bool onGround = false;
	bool alive = true;
	bool underground = false;
};

class DroppedItemState
{
public:
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

class LeafParticleState
{
public:
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

class MinimapRunState
{
public:
	float x = 0.0f;
	float y = 0.0f;
	float width = 0.0f;
	float colorR = 0.0f;
	float colorG = 0.0f;
	float colorB = 0.0f;
};

class RemotePlayerState
{
public:
	PlayerState player;
	std::array<char, 24> nickname = {};
	float lastHeardTime = 0.0f;
	float attackTimer = 0.0f;
	int id = 0;
	int health = 100;
	int selectedInventorySlot = 0;
	bool active = false;
};

class RemoteBlockBreakState
{
public:
	float progress = 0.0f;
	float lastHeardTime = 0.0f;
	int playerId = 0;
	int tileX = 0;
	int tileY = 0;
	bool active = false;
};

class NetworkPeerState
{
public:
	NetworkAddress address = {};
	float lastHeardTime = 0.0f;
	unsigned int token = 0;
	int playerId = 0;
	bool active = false;
};

class NetworkTileEditState
{
public:
	int tileX = 0;
	int tileY = 0;
	unsigned short tileIndex = 0;
	unsigned char visible = 0;
	unsigned int sequence = 0;
};
