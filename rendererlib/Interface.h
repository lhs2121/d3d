#pragma once
#include "d3d11.h"
#include "Windows.h"

#ifdef RENDERERLIB_EXPORTS
#define RENDERERLIB_API __declspec(dllexport)
#else
#define RENDERERLIB_API __declspec(dllimport)
#endif 

struct BlockTile
{
	unsigned short tileIndex = 0;
	unsigned char visible = 1;
	unsigned char reserved = 0;
};

struct BlockGridDesc
{
	const WCHAR* textureFile = nullptr;
	const BlockTile* tiles = nullptr;
	int width = 0;
	int height = 0;
	int atlasColumns = 1;
	int atlasRows = 1;
	float tileSize = 16.0f;
	float originX = 0.0f;
	float originY = 0.0f;
	const unsigned int* chunkVersions = nullptr;
	int chunkSizeTiles = 16;
	int chunkColumns = 0;
	int chunkRows = 0;
	unsigned int gridVersion = 0;
};

struct SpriteDesc
{
	const WCHAR* textureFile = nullptr;
	float positionX = 0.0f;
	float positionY = 0.0f;
	float width = 16.0f;
	float height = 16.0f;
	int atlasColumns = 1;
	int atlasRows = 1;
	int tileIndex = 0;
	int flipX = 0;
	float rotationRadians = 0.0f;
	float colorR = 1.0f;
	float colorG = 1.0f;
	float colorB = 1.0f;
	float colorA = 1.0f;
	float depth = 0.0f;
};

struct RectOutlineDesc
{
	float positionX = 0.0f;
	float positionY = 0.0f;
	float width = 16.0f;
	float height = 16.0f;
	float thickness = 2.0f;
	float colorR = 1.0f;
	float colorG = 1.0f;
	float colorB = 1.0f;
	float colorA = 1.0f;
	float depth = -0.5f;
};

struct GlyphSpriteDesc
{
	float positionX = 0.0f;
	float positionY = 0.0f;
	float width = 10.0f;
	float height = 14.0f;
	unsigned char rows[7] = {};
	float colorR = 1.0f;
	float colorG = 1.0f;
	float colorB = 1.0f;
	float colorA = 1.0f;
	float depth = -8.0f;
};

struct RenderFrameStats
{
	unsigned int drawCalls = 0;
	unsigned int spriteDrawCalls = 0;
	unsigned int gridDrawCalls = 0;
	unsigned int gridInstances = 0;
	unsigned int gridChunksDrawn = 0;
	unsigned int gridChunksRebuilt = 0;
	unsigned int spriteQuads = 0;
	unsigned int rectOutlineCalls = 0;
	unsigned int textureBinds = 0;
};

struct IRenderer
{
	virtual void Initialize(UINT winSizeX, UINT winSizeY, HWND& hwnd) = 0;

	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;

	virtual void LoadTexture(const WCHAR* textureFile) = 0;
	virtual void DrawBlockGrid(const BlockGridDesc& desc) = 0;
	virtual void DrawSprite(const SpriteDesc& desc) = 0;
	virtual void DrawRectOutline(const RectOutlineDesc& desc) = 0;
	virtual void DrawGlyphSprite(const GlyphSpriteDesc& desc) = 0;
	virtual void GetLastFrameStats(RenderFrameStats& outStats) const = 0;
};

extern "C" RENDERERLIB_API void CreateRenderer(IRenderer** ppRenderer);
extern "C" RENDERERLIB_API void DeleteRenderer(IRenderer* pRenderer);
