#pragma once
#include "Interface.h"
#include "d3d.h"
#include "d3dcompiler.h"
#include <string>
#include <unordered_map>
#include <vector>

using namespace DirectX;

struct RenderPipeline
{
	ID3D11Buffer* pQuadVertexBuffer = nullptr;
	ID3D11Buffer* pQuadIndexBuffer = nullptr;
	ID3D11InputLayout* pInputLayout = nullptr;
	ID3D11SamplerState* pSampler = nullptr;
	ID3D11RasterizerState* pRasterizer = nullptr;
	ID3D11DepthStencilState* pDepthStencilState = nullptr;
	ID3D11BlendState* pBlendState = nullptr;
	ID3D11VertexShader* pVertexShader = nullptr;
	ID3D11PixelShader* pPixelShader = nullptr;
	ID3D11ShaderResourceView* pTexture = nullptr;
	ID3D11Buffer* pTransformConstantBuffer = nullptr;
	ID3D11Buffer* pSpriteConstantBuffer = nullptr;
	ID3D11Buffer* pGridVertexBuffer = nullptr;
	ID3D11Buffer* pGridIndexBuffer = nullptr;
	ID3D11ShaderResourceView* pWhiteTexture = nullptr;
};

class Renderer : public IRenderer
{
public:
	~Renderer();
	void Initialize(UINT winWidth, UINT winHeight, HWND& hwnd) override;
	void BeginFrame() override;
	void EndFrame() override;

	void LoadTexture(const WCHAR* textureFile) override;
	void DrawBlockGrid(const BlockGridDesc& desc) override;
	void DrawSprite(const SpriteDesc& desc) override;
	void DrawRectOutline(const RectOutlineDesc& desc) override;

private:
	void InitializePipeline();
	void ReleasePipeline();
	void ReleaseTextures();
	void CreateWhiteTexture();
	void LoadPipelineShader(const WCHAR* shaderFile);
	ID3D11ShaderResourceView* GetTexture(const WCHAR* textureFile);
	void BindSpritePipeline();
	void BindTexture(ID3D11ShaderResourceView* texture);
	void EnsureGridBatchCapacity(size_t quadCount);
	void DrawSpriteQuad(const SpriteDesc& desc, ID3D11ShaderResourceView* texture);

	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pDeviceContext = nullptr;
	IDXGISwapChain* m_pSwapChain = nullptr;
	ID3D11Texture2D* m_pRenderTargetBuffer = nullptr;
	ID3D11Texture2D* m_pDepthStencilBuffer = nullptr;
	ID3D11DepthStencilView* m_pDepthStencilView = nullptr;
	ID3D11RenderTargetView* m_pRenderTargetView = nullptr;
	ID3DBlob* m_pVertexShaderBlob = nullptr;
	RenderPipeline m_pipeline;
	std::unordered_map<std::wstring, ID3D11ShaderResourceView*> m_textureMap;
	std::vector<SimpleVertex> m_gridVertices;
	std::vector<USHORT> m_gridIndices;
	ID3D11ShaderResourceView* m_boundTexture = nullptr;
	bool m_spritePipelineBound = false;
	size_t m_gridQuadCapacity = 0;
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
