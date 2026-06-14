#pragma once

#include <cstddef>

constexpr unsigned int GameNetworkMagic = 0x4433444Du;
constexpr unsigned char GameNetworkVersion = 1;
constexpr int GameNetworkNicknameLength = 24;

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
	unsigned int magic = GameNetworkMagic;
	unsigned char version = GameNetworkVersion;
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
	char nickname[GameNetworkNicknameLength] = {};
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

inline NetworkPacketHeader MakeNetworkHeader(NetworkPacketType type, size_t packetSize)
{
	NetworkPacketHeader header;
	header.magic = GameNetworkMagic;
	header.version = GameNetworkVersion;
	header.type = static_cast<unsigned char>(type);
	header.size = static_cast<unsigned short>(packetSize);
	return header;
}
