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
	const unsigned short* atlasTileRemap = nullptr;
	int atlasTileRemapCount = 0;
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
	float uvX = 0.0f;
	float uvY = 0.0f;
	float uvWidth = 0.0f;
	float uvHeight = 0.0f;
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

struct TextDesc
{
	const char* text = nullptr;
	float x = 0.0f;
	float y = 0.0f;
	float pixelSize = 2.0f;
	float colorR = 1.0f;
	float colorG = 1.0f;
	float colorB = 1.0f;
	float colorA = 1.0f;
	float depth = -8.0f;
};

struct RenderFrameStats
{
	unsigned int backBufferWidth = 0;
	unsigned int backBufferHeight = 0;
	unsigned int viewportChanges = 0;
	unsigned int drawCalls = 0;
	unsigned int spriteDrawCalls = 0;
	unsigned int fontDrawCalls = 0;
	unsigned int glyphDrawCalls = 0;
	unsigned int gridDrawCalls = 0;
	unsigned int gridInstances = 0;
	unsigned int gridVisibleColumns = 0;
	unsigned int gridVisibleRows = 0;
	unsigned int gridVisibleTiles = 0;
	unsigned int gridChunksDrawn = 0;
	unsigned int gridChunksRebuilt = 0;
	unsigned int gridCacheInvalidations = 0;
	unsigned int spriteQuads = 0;
	unsigned int whiteQuads = 0;
	unsigned int texturedQuads = 0;
	unsigned int fontQuads = 0;
	unsigned int glyphQuads = 0;
	unsigned int spriteBatches = 0;
	unsigned int fontBatches = 0;
	unsigned int glyphBatches = 0;
	unsigned int maxSpriteBatchQuads = 0;
	unsigned int maxFontBatchQuads = 0;
	unsigned int maxGlyphBatchQuads = 0;
	unsigned int textDrawCalls = 0;
	unsigned int textGlyphs = 0;
	unsigned int fontGlyphsCached = 0;
	unsigned int fontAtlasUploads = 0;
	unsigned int fontAtlasUploadBytes = 0;
	unsigned int rectOutlineCalls = 0;
	unsigned int textureBinds = 0;
	unsigned int dynamicBufferUploads = 0;
	unsigned int dynamicBufferUploadBytes = 0;
	unsigned int gridUploadBytes = 0;
	unsigned int spriteUploadBytes = 0;
	unsigned int fontUploadBytes = 0;
	unsigned int glyphUploadBytes = 0;
};

struct IRenderer
{
	virtual void Initialize(UINT winSizeX, UINT winSizeY, HWND& hwnd) = 0;
	virtual void Resize(UINT winSizeX, UINT winSizeY) = 0;

	virtual void BeginFrame() = 0;
	virtual void EndFrame() = 0;
	virtual void SetViewportRect(float left, float top, float width, float height) = 0;
	virtual void ResetViewportRect() = 0;

	virtual void LoadTexture(const WCHAR* textureFile) = 0;
	virtual int CreateDynamicTexture(int width, int height, const unsigned int* pixels) = 0;
	virtual void UpdateDynamicTexture(int textureId, int x, int y, int width, int height, const unsigned int* pixels, int pitchPixels) = 0;
	virtual void DrawDynamicTexture(int textureId, const SpriteDesc& desc) = 0;
	virtual void ReleaseDynamicTexture(int textureId) = 0;
	virtual void DrawBlockGrid(const BlockGridDesc& desc) = 0;
	virtual void DrawSprite(const SpriteDesc& desc) = 0;
	virtual void DrawRectOutline(const RectOutlineDesc& desc) = 0;
	virtual void DrawGlyphSprite(const GlyphSpriteDesc& desc) = 0;
	virtual void DrawText(const TextDesc& desc) = 0;
	virtual void GetLastFrameStats(RenderFrameStats& outStats) const = 0;
};

extern "C" RENDERERLIB_API void CreateRenderer(IRenderer** ppRenderer);
extern "C" RENDERERLIB_API void DeleteRenderer(IRenderer* pRenderer);
