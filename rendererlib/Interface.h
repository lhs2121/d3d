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
	float depth = 0.0f;
};

struct RectOutlineDesc
{
	float positionX = 0.0f;
	float positionY = 0.0f;
	float width = 16.0f;
	float height = 16.0f;
	float thickness = 2.0f;
	float depth = -0.5f;
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
};

extern "C" RENDERERLIB_API void CreateRenderer(IRenderer** ppRenderer);
extern "C" RENDERERLIB_API void DeleteRenderer(IRenderer* pRenderer);
