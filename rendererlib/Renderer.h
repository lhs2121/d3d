#pragma once
#include "Interface.h"
#include "d3d.h"
#include "d3dcompiler.h"
#include <string>
#include <unordered_map>
#include <vector>

using namespace DirectX;

struct GridInstance
{
	math::Vec4 transform;
	math::Vec4 uv;
};

struct GridChunkCache
{
	std::vector<GridInstance> instances;
	unsigned int version = 0;
	int startX = 0;
	int startY = 0;
	int endX = -1;
	int endY = -1;
	bool valid = false;
};

struct GridCacheState
{
	const BlockTile* tiles = nullptr;
	int width = 0;
	int height = 0;
	int atlasColumns = 0;
	int atlasRows = 0;
	int chunkSizeTiles = 0;
	int chunkColumns = 0;
	int chunkRows = 0;
	unsigned int gridVersion = 0;
	float tileSize = 0.0f;
	std::vector<GridChunkCache> chunks;
};

struct SpriteBatchInstance
{
	math::Vec4 transform;
	math::Vec4 rotationDepth;
	math::Vec4 uv;
	math::Vec4 color;
};

struct GlyphBatchInstance
{
	math::Vec4 transform;
	math::Vec4 rows0;
	math::Vec4 rows1Depth;
	math::Vec4 color;
};

struct FontGlyph
{
	int tileIndex = 0;
	float widthPixels = 0.0f;
	float advancePixels = 0.0f;
};

struct RenderPipeline
{
	ID3D11Buffer* pQuadVertexBuffer = nullptr;
	ID3D11Buffer* pQuadIndexBuffer = nullptr;
	ID3D11InputLayout* pInputLayout = nullptr;
	ID3D11InputLayout* pGridInputLayout = nullptr;
	ID3D11InputLayout* pSpriteBatchInputLayout = nullptr;
	ID3D11InputLayout* pGlyphBatchInputLayout = nullptr;
	ID3D11SamplerState* pSampler = nullptr;
	ID3D11RasterizerState* pRasterizer = nullptr;
	ID3D11DepthStencilState* pDepthStencilState = nullptr;
	ID3D11BlendState* pBlendState = nullptr;
	ID3D11VertexShader* pVertexShader = nullptr;
	ID3D11VertexShader* pGridVertexShader = nullptr;
	ID3D11VertexShader* pSpriteBatchVertexShader = nullptr;
	ID3D11VertexShader* pGlyphBatchVertexShader = nullptr;
	ID3D11PixelShader* pPixelShader = nullptr;
	ID3D11PixelShader* pSpriteBatchPixelShader = nullptr;
	ID3D11PixelShader* pGlyphBatchPixelShader = nullptr;
	ID3D11ShaderResourceView* pTexture = nullptr;
	ID3D11Buffer* pTransformConstantBuffer = nullptr;
	ID3D11Buffer* pSpriteConstantBuffer = nullptr;
	ID3D11Buffer* pGridInstanceBuffer = nullptr;
	ID3D11Buffer* pSpriteBatchInstanceBuffer = nullptr;
	ID3D11Buffer* pGlyphBatchInstanceBuffer = nullptr;
	ID3D11ShaderResourceView* pWhiteTexture = nullptr;
};

class Renderer : public IRenderer
{
public:
	~Renderer();
	void Initialize(UINT winWidth, UINT winHeight, HWND& hwnd) override;
	void Resize(UINT winWidth, UINT winHeight) override;
	void BeginFrame() override;
	void EndFrame() override;
	void SetViewportRect(float left, float top, float width, float height) override;
	void ResetViewportRect() override;

	void LoadTexture(const WCHAR* textureFile) override;
	void DrawBlockGrid(const BlockGridDesc& desc) override;
	void DrawSprite(const SpriteDesc& desc) override;
	void DrawRectOutline(const RectOutlineDesc& desc) override;
	void DrawGlyphSprite(const GlyphSpriteDesc& desc) override;
	void DrawText(const TextDesc& desc) override;
	void GetLastFrameStats(RenderFrameStats& outStats) const override;

private:
	void InitializePipeline();
	void ReleasePipeline();
	void ReleaseTextures();
	void ReleaseFontAtlas();
	void CreateWhiteTexture();
	bool EnsureFontAtlas();
	bool EnsureFontGlyph(unsigned int codepoint);
	void UploadFontAtlas();
	unsigned int DecodeUtf8Codepoint(const char*& cursor) const;
	void LoadPipelineShader(const WCHAR* shaderFile);
	ID3D11ShaderResourceView* GetTexture(const WCHAR* textureFile);
	void BindSpritePipeline();
	void BindTexture(ID3D11ShaderResourceView* texture);
	void EnsureGridBatchCapacity(size_t quadCount);
	void EnsureSpriteBatchCapacity(size_t quadCount);
	void EnsureGlyphBatchCapacity(size_t quadCount);
	void ResetGridCache();
	void RebuildGridChunk(const BlockGridDesc& desc, GridChunkCache& chunk, int chunkX, int chunkY,
		int chunkSize, int atlasColumns, int atlasRows, int tileCount, float uvWidth, float uvHeight);
	SpriteBatchInstance BuildSpriteBatchInstance(const SpriteDesc& desc) const;
	void QueueSprite(ID3D11ShaderResourceView* texture, const SpriteDesc& desc);
	void FlushSpriteBatch();
	void FlushFontSpriteBatch();
	void FlushGlyphBatch();
	void DrawSpriteQuad(const SpriteDesc& desc, ID3D11ShaderResourceView* texture);

	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pDeviceContext = nullptr;
	IDXGISwapChain* m_pSwapChain = nullptr;
	ID3D11Texture2D* m_pRenderTargetBuffer = nullptr;
	ID3D11Texture2D* m_pDepthStencilBuffer = nullptr;
	ID3D11DepthStencilView* m_pDepthStencilView = nullptr;
	ID3D11RenderTargetView* m_pRenderTargetView = nullptr;
	ID3DBlob* m_pVertexShaderBlob = nullptr;
	ID3DBlob* m_pGridVertexShaderBlob = nullptr;
	ID3DBlob* m_pSpriteBatchVertexShaderBlob = nullptr;
	ID3DBlob* m_pGlyphBatchVertexShaderBlob = nullptr;
	RenderPipeline m_pipeline;
	std::unordered_map<std::wstring, ID3D11ShaderResourceView*> m_textureMap;
	std::vector<GridInstance> m_gridInstances;
	GridCacheState m_gridCache;
	std::vector<SpriteBatchInstance> m_spriteBatchInstances;
	std::vector<SpriteBatchInstance> m_fontSpriteBatchInstances;
	std::vector<GlyphBatchInstance> m_glyphBatchInstances;
	std::unordered_map<unsigned int, FontGlyph> m_fontGlyphs;
	std::vector<unsigned int> m_fontAtlasPixels;
	RenderFrameStats m_currentFrameStats;
	RenderFrameStats m_lastFrameStats;
	const WCHAR* m_lastTextureFile = nullptr;
	ID3D11ShaderResourceView* m_lastTextureView = nullptr;
	ID3D11ShaderResourceView* m_boundTexture = nullptr;
	ID3D11ShaderResourceView* m_spriteBatchTexture = nullptr;
	ID3D11Texture2D* m_fontAtlasTexture = nullptr;
	ID3D11ShaderResourceView* m_fontAtlasView = nullptr;
	HFONT m_fontHandle = nullptr;
	HDC m_fontDc = nullptr;
	HBITMAP m_fontBitmap = nullptr;
	void* m_fontBitmapPixels = nullptr;
	bool m_spritePipelineBound = false;
	bool m_fontAtlasReady = false;
	bool m_fontAtlasDirty = false;
	size_t m_gridQuadCapacity = 0;
	size_t m_spriteBatchQuadCapacity = 0;
	size_t m_glyphBatchQuadCapacity = 0;
	int m_fontAtlasWidth = 1024;
	int m_fontAtlasHeight = 1024;
	int m_fontAtlasCellWidth = 48;
	int m_fontAtlasCellHeight = 48;
	int m_fontAtlasColumns = 0;
	int m_fontAtlasRows = 0;
	int m_fontGlyphCount = 0;
	int m_fontPadding = 4;
	float m_fontAsciiWidth = 27.0f;
	UINT m_windowWidth = 1;
	UINT m_windowHeight = 1;
	float m_viewHalfWidth = 1.0f;
	float m_viewHalfHeight = 1.0f;

	FLOAT m_clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	XMMATRIX m_matView;
	XMMATRIX m_matProjection;

	XMVECTOR m_CameraPosition = { 0.0f, 0.0f, -500.0f, 1.0f };
	XMVECTOR m_CameraEyeDirection = { 0.0f ,0.0f, 1.0f, 1.0f };
	XMVECTOR m_CameraUpDirection = { 0.0f ,1.0f, 0.0f, 1.0f };
	float m_degFovY = 60.0f;
	float m_near = 0.3f;	
	float m_far = 1000.0f;
};
