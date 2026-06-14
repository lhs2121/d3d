#include "pch.h"
#include "Renderer.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

using namespace DirectX;

#pragma comment(lib, "gdi32.lib")

struct SpriteTransformData
{
	XMMATRIX matWorld;
	XMMATRIX matView;
	XMMATRIX matProjection;
};

namespace
{
	constexpr size_t VerticesPerQuad = 4;
	constexpr int FontAtlasTextureSize = 2048;
	constexpr int FontAtlasCellSize = 64;
	constexpr int FontAtlasPixelHeight = 32;
	constexpr const WCHAR* FontFileName = L"assets\\fonts\\GalmuriMono9.ttf";
	constexpr const WCHAR* FontFamilyName = L"GalmuriMono9 Regular";
	constexpr size_t ShaderEntryNameCapacity = 256;

	ID3D11Buffer* CreateDynamicBuffer(ID3D11Device* pDevice, UINT byteWidth, UINT bindFlags)
	{
		ID3D11Buffer* pBuffer = nullptr;

		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = byteWidth;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = bindFlags;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		if (S_OK != pDevice->CreateBuffer(&desc, nullptr, &pBuffer))
			__debugbreak();

		return pBuffer;
	}

	bool ResolveWorkspaceFile(const WCHAR* fileName, WCHAR* resolvedPath, DWORD resolvedPathCount)
	{
		if (fileName == nullptr || resolvedPath == nullptr || resolvedPathCount == 0)
			return false;

		GetCurrentDirectoryW(resolvedPathCount, resolvedPath);
		PathAppendW(resolvedPath, fileName);
		if (PathFileExistsW(resolvedPath))
			return true;

		WCHAR moduleDir[MAX_PATH] = {};
		GetModuleFileNameW(nullptr, moduleDir, MAX_PATH);
		PathRemoveFileSpecW(moduleDir);
		PathAppendW(moduleDir, L"..\\..\\..");
		PathCanonicalizeW(resolvedPath, moduleDir);
		PathAppendW(resolvedPath, fileName);
		return PathFileExistsW(resolvedPath) != FALSE;
	}

	bool GetShaderBaseName(const WCHAR* shaderFile, char* baseName, size_t baseNameCapacity)
	{
		if (shaderFile == nullptr || baseName == nullptr || baseNameCapacity == 0)
			return false;

		baseName[0] = '\0';
		const WCHAR* fileName = PathFindFileNameW(shaderFile);
		WideCharToMultiByte(CP_UTF8, 0, fileName, -1, baseName, static_cast<int>(baseNameCapacity), nullptr, nullptr);
		baseName[baseNameCapacity - 1] = '\0';

		char* extension = std::strchr(baseName, '.');
		if (extension == nullptr || extension == baseName)
			return false;

		*extension = '\0';
		return true;
	}

	void BuildShaderEntryName(char* entryName, size_t entryNameCapacity, const char* baseName, const char* suffix)
	{
		if (entryName == nullptr || entryNameCapacity == 0)
			return;

		entryName[0] = '\0';
		strcat_s(entryName, entryNameCapacity, baseName);
		strcat_s(entryName, entryNameCapacity, suffix);
	}

	ID3DBlob* CompileShaderEntry(const WCHAR* shaderPath, const char* entryName, const char* target, int flags)
	{
		ID3DBlob* shaderBlob = nullptr;
		ID3DBlob* errorBlob = nullptr;
		const HRESULT hr = D3DCompileFromFile(
			shaderPath,
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entryName,
			target,
			flags,
			0,
			&shaderBlob,
			&errorBlob);

		if (FAILED(hr))
		{
			if (errorBlob != nullptr)
			{
				OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));
				errorBlob->Release();
			}
			else
			{
				OutputDebugStringA("Shader compile failed, but no error blob was returned: ");
				OutputDebugStringA(entryName);
				OutputDebugStringA("\n");
				OutputDebugStringW(shaderPath);
				OutputDebugStringW(L"\n");
			}

			__debugbreak();
			return nullptr;
		}

		return shaderBlob;
	}
}

Renderer::~Renderer()
{
	ReleasePipeline();

	if (m_pDeviceContext)
		m_pDeviceContext->ClearState();

	if (m_pDeviceContext)
		m_pDeviceContext->Release();
	if (m_pSwapChain)
		m_pSwapChain->Release();
	if (m_pRenderTargetBuffer)
		m_pRenderTargetBuffer->Release();
	if (m_pDepthStencilBuffer)
		m_pDepthStencilBuffer->Release();
	if (m_pDepthStencilView)
		m_pDepthStencilView->Release();
	if (m_pRenderTargetView)
		m_pRenderTargetView->Release();

#if defined(DEBUG) || defined(_DEBUG)
	ID3D11Debug* dxgiDebug;

	if (m_pDevice && S_OK == m_pDevice->QueryInterface(IID_PPV_ARGS(&dxgiDebug)))
	{
		dxgiDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
		dxgiDebug->Release();
	}
#endif

	if (m_pDevice)
		m_pDevice->Release();
}

void Renderer::Initialize(UINT winWidth, UINT winHeight, HWND& hwnd)
{
	m_windowWidth = winWidth > 0 ? winWidth : 1;
	m_windowHeight = winHeight > 0 ? winHeight : 1;

	IDXGIFactory* pFact = nullptr;
	IDXGIAdapter* pAdap = nullptr;

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG) && defined(ENABLE_D3D11_DEBUG_LAYER)
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFact));
	pFact->EnumAdapters(0, &pAdap);

	HRESULT rs = D3D11CreateDevice(pAdap, D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_UNKNOWN, nullptr,
		deviceFlags, nullptr, 0, D3D11_SDK_VERSION, &m_pDevice, &featureLevel, &m_pDeviceContext);
	if (rs != S_OK)
		__debugbreak();

	{
		DXGI_SWAP_CHAIN_DESC desc = { 0 };
		desc.BufferDesc.Width = winWidth;
		desc.BufferDesc.Height = winHeight;
		desc.BufferDesc.RefreshRate.Numerator = 60;
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
		desc.BufferCount = 2;
		desc.OutputWindow = hwnd;
		desc.Windowed = true;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		if (S_OK != pFact->CreateSwapChain(m_pDevice, &desc, &m_pSwapChain))
			__debugbreak();
	}

	if (S_OK != m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer)))
		__debugbreak();

	if (S_OK != m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView))
		__debugbreak();


	D3D11_TEXTURE2D_DESC dsbDesc;
	dsbDesc.Width = winWidth;
	dsbDesc.Height = winHeight;
	dsbDesc.MipLevels = 1;
	dsbDesc.ArraySize = 1;
	dsbDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsbDesc.SampleDesc.Count = 1;
	dsbDesc.SampleDesc.Quality = 0;
	dsbDesc.Usage = D3D11_USAGE_DEFAULT;
	dsbDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	dsbDesc.CPUAccessFlags = 0;
	dsbDesc.MiscFlags = 0;

	if (S_OK != m_pDevice->CreateTexture2D(&dsbDesc, nullptr, &m_pDepthStencilBuffer))
		__debugbreak();

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format = dsbDesc.Format;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	if (S_OK != m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &dsvDesc, &m_pDepthStencilView))
		__debugbreak();

	D3D11_VIEWPORT viewportDesc;
	viewportDesc.Width = (FLOAT)winWidth;
	viewportDesc.Height = (FLOAT)winHeight;
	viewportDesc.MinDepth = 0;
	viewportDesc.MaxDepth = 1;
	viewportDesc.TopLeftX = 0;
	viewportDesc.TopLeftY = 0;
	m_pDeviceContext->RSSetViewports(1, &viewportDesc);

	pFact->Release();
	pAdap->Release();

	m_matView = XMMatrixLookToLH(m_CameraPosition, m_CameraEyeDirection, m_CameraUpDirection);
	m_matProjection = XMMatrixPerspectiveFovLH(m_degFovY * Deg2Rad, (float)winWidth / (float)winHeight, m_near, m_far);
	const float cameraDistance = std::fabs(XMVectorGetZ(m_CameraPosition));
	m_viewHalfHeight = std::tan((m_degFovY * Deg2Rad) * 0.5f) * cameraDistance;
	m_viewHalfWidth = m_viewHalfHeight * ((float)winWidth / (float)winHeight);

	InitializePipeline();
	m_gridInstances.reserve(4096);
	m_spriteBatchInstances.reserve(1024);
	m_fontSpriteBatchInstances.reserve(1024);
	m_glyphBatchInstances.reserve(256);

	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->IASetInputLayout(m_pipeline.pInputLayout);

}

void Renderer::Resize(UINT winWidth, UINT winHeight)
{
	winWidth = winWidth > 0 ? winWidth : 1;
	winHeight = winHeight > 0 ? winHeight : 1;
	if (m_windowWidth == winWidth && m_windowHeight == winHeight &&
		m_pRenderTargetView != nullptr && m_pDepthStencilView != nullptr)
	{
		return;
	}

	if (m_pSwapChain == nullptr || m_pDevice == nullptr || m_pDeviceContext == nullptr)
		return;

	FlushSpriteBatch();
	FlushFontSpriteBatch();
	FlushGlyphBatch();

	ID3D11RenderTargetView* nullRenderTargets[] = { nullptr };
	m_pDeviceContext->OMSetRenderTargets(1, nullRenderTargets, nullptr);

	if (m_pRenderTargetView)
	{
		m_pRenderTargetView->Release();
		m_pRenderTargetView = nullptr;
	}
	if (m_pRenderTargetBuffer)
	{
		m_pRenderTargetBuffer->Release();
		m_pRenderTargetBuffer = nullptr;
	}
	if (m_pDepthStencilView)
	{
		m_pDepthStencilView->Release();
		m_pDepthStencilView = nullptr;
	}
	if (m_pDepthStencilBuffer)
	{
		m_pDepthStencilBuffer->Release();
		m_pDepthStencilBuffer = nullptr;
	}

	const HRESULT resizeResult = m_pSwapChain->ResizeBuffers(
		0,
		winWidth,
		winHeight,
		DXGI_FORMAT_UNKNOWN,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
	if (FAILED(resizeResult))
		__debugbreak();

	m_windowWidth = winWidth;
	m_windowHeight = winHeight;

	if (S_OK != m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer)))
		__debugbreak();
	if (S_OK != m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView))
		__debugbreak();

	D3D11_TEXTURE2D_DESC dsbDesc = {};
	dsbDesc.Width = winWidth;
	dsbDesc.Height = winHeight;
	dsbDesc.MipLevels = 1;
	dsbDesc.ArraySize = 1;
	dsbDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsbDesc.SampleDesc.Count = 1;
	dsbDesc.SampleDesc.Quality = 0;
	dsbDesc.Usage = D3D11_USAGE_DEFAULT;
	dsbDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	if (S_OK != m_pDevice->CreateTexture2D(&dsbDesc, nullptr, &m_pDepthStencilBuffer))
		__debugbreak();

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = dsbDesc.Format;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	if (S_OK != m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &dsvDesc, &m_pDepthStencilView))
		__debugbreak();

	m_matView = XMMatrixLookToLH(m_CameraPosition, m_CameraEyeDirection, m_CameraUpDirection);
	m_matProjection = XMMatrixPerspectiveFovLH(m_degFovY * Deg2Rad, static_cast<float>(winWidth) / static_cast<float>(winHeight), m_near, m_far);
	const float cameraDistance = std::fabs(XMVectorGetZ(m_CameraPosition));
	m_viewHalfHeight = std::tan((m_degFovY * Deg2Rad) * 0.5f) * cameraDistance;
	m_viewHalfWidth = m_viewHalfHeight * (static_cast<float>(winWidth) / static_cast<float>(winHeight));

	ResetViewportRect();
	m_boundTexture = nullptr;
	m_spriteBatchTexture = nullptr;
	m_spritePipelineBound = false;
}

void Renderer::BeginFrame()
{
	m_currentFrameStats = RenderFrameStats();
	m_currentFrameStats.backBufferWidth = m_windowWidth;
	m_currentFrameStats.backBufferHeight = m_windowHeight;
	m_currentFrameStats.fontGlyphsCached = static_cast<unsigned int>(m_fontGlyphCount);
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, m_clearColor);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);
	ResetViewportRect();
	m_spritePipelineBound = false;
	m_boundTexture = nullptr;
	m_spriteBatchTexture = nullptr;
}

void Renderer::EndFrame()
{
	FlushSpriteBatch();
	FlushFontSpriteBatch();
	FlushGlyphBatch();
	m_lastFrameStats = m_currentFrameStats;
	m_pSwapChain->Present(0, 0);
}

void Renderer::SetViewportRect(float left, float top, float width, float height)
{
	FlushSpriteBatch();
	FlushFontSpriteBatch();
	FlushGlyphBatch();

	const float viewWidth = m_viewHalfWidth * 2.0f;
	const float viewHeight = m_viewHalfHeight * 2.0f;
	if (viewWidth <= 0.0f || viewHeight <= 0.0f || width <= 0.0f || height <= 0.0f)
		return;

	const float right = left + width;
	const float bottom = top - height;
	const float normalizedLeft = (left + m_viewHalfWidth) / viewWidth;
	const float normalizedRight = (right + m_viewHalfWidth) / viewWidth;
	const float normalizedTop = (m_viewHalfHeight - top) / viewHeight;
	const float normalizedBottom = (m_viewHalfHeight - bottom) / viewHeight;

	LONG pixelLeft = static_cast<LONG>(std::floor(normalizedLeft * static_cast<float>(m_windowWidth)));
	LONG pixelRight = static_cast<LONG>(std::ceil(normalizedRight * static_cast<float>(m_windowWidth)));
	LONG pixelTop = static_cast<LONG>(std::floor(normalizedTop * static_cast<float>(m_windowHeight)));
	LONG pixelBottom = static_cast<LONG>(std::ceil(normalizedBottom * static_cast<float>(m_windowHeight)));

	pixelLeft = std::clamp<LONG>(pixelLeft, 0, static_cast<LONG>(m_windowWidth));
	pixelRight = std::clamp<LONG>(pixelRight, 0, static_cast<LONG>(m_windowWidth));
	pixelTop = std::clamp<LONG>(pixelTop, 0, static_cast<LONG>(m_windowHeight));
	pixelBottom = std::clamp<LONG>(pixelBottom, 0, static_cast<LONG>(m_windowHeight));
	if (pixelRight <= pixelLeft || pixelBottom <= pixelTop)
		return;

	D3D11_VIEWPORT viewportDesc = {};
	viewportDesc.Width = static_cast<FLOAT>(pixelRight - pixelLeft);
	viewportDesc.Height = static_cast<FLOAT>(pixelBottom - pixelTop);
	viewportDesc.MinDepth = 0.0f;
	viewportDesc.MaxDepth = 1.0f;
	viewportDesc.TopLeftX = static_cast<FLOAT>(pixelLeft);
	viewportDesc.TopLeftY = static_cast<FLOAT>(pixelTop);
	m_pDeviceContext->RSSetViewports(1, &viewportDesc);

	D3D11_RECT scissorRect = {};
	scissorRect.left = pixelLeft;
	scissorRect.top = pixelTop;
	scissorRect.right = pixelRight;
	scissorRect.bottom = pixelBottom;
	m_pDeviceContext->RSSetScissorRects(1, &scissorRect);
	++m_currentFrameStats.viewportChanges;
}

void Renderer::ResetViewportRect()
{
	FlushSpriteBatch();
	FlushFontSpriteBatch();
	FlushGlyphBatch();

	D3D11_VIEWPORT viewportDesc = {};
	viewportDesc.Width = static_cast<FLOAT>(m_windowWidth);
	viewportDesc.Height = static_cast<FLOAT>(m_windowHeight);
	viewportDesc.MinDepth = 0.0f;
	viewportDesc.MaxDepth = 1.0f;
	viewportDesc.TopLeftX = 0.0f;
	viewportDesc.TopLeftY = 0.0f;
	m_pDeviceContext->RSSetViewports(1, &viewportDesc);

	D3D11_RECT scissorRect = {};
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = static_cast<LONG>(m_windowWidth);
	scissorRect.bottom = static_cast<LONG>(m_windowHeight);
	m_pDeviceContext->RSSetScissorRects(1, &scissorRect);
	++m_currentFrameStats.viewportChanges;
}

void Renderer::InitializePipeline()
{
	SimpleVertex Rect2D[] =
	{
		{ math::Vec4(-0.5f, 0.5f, 0.0f, 1.0f), math::Vec2(0.0f, 0.0f) },
		{ math::Vec4(0.5f, 0.5f, 0.0f, 1.0f), math::Vec2(1.0f, 0.0f) },
		{ math::Vec4(0.5f,-0.5f, 0.0f, 1.0f), math::Vec2(1.0f, 1.0f) },
		{ math::Vec4(-0.5f,-0.5f, 0.0f, 1.0f), math::Vec2(0.0f, 1.0f) }
	};

	USHORT Rect2DIndex[]
	{
		0,1,2,
		0,2,3
	};

	m_pipeline.pQuadVertexBuffer = d3d::CreateVertexBuffer(m_pDevice, Rect2D, sizeof(Rect2D), sizeof(SimpleVertex));
	m_pipeline.pQuadIndexBuffer = d3d::CreateIndexBuffer(m_pDevice, Rect2DIndex, sizeof(Rect2DIndex), sizeof(USHORT));
	m_pipeline.pRasterizer = d3d::CreateRasterizerState(m_pDevice);
	m_pipeline.pSampler = d3d::CreateSamplerState(m_pDevice);
	m_pipeline.pBlendState = d3d::CreateBlendState(m_pDevice);
	m_pipeline.pDepthStencilState = d3d::CreateDepthStencilState(m_pDevice);

	LoadPipelineShader(L"assets\\shaders\\BasicSprite2DShader.hlsl");
	CreateWhiteTexture();

	D3D11_INPUT_ELEMENT_DESC iaDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "NORMAL",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0 },
	};

	if (S_OK != m_pDevice->CreateInputLayout(iaDesc, 2, m_pVertexShaderBlob->GetBufferPointer(), m_pVertexShaderBlob->GetBufferSize(), &m_pipeline.pInputLayout))
		__debugbreak();

	m_pVertexShaderBlob->Release();
	m_pVertexShaderBlob = nullptr;

	D3D11_INPUT_ELEMENT_DESC gridIaDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "TILE",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,0,D3D11_INPUT_PER_INSTANCE_DATA,1 },
		{ "TILE",1,DXGI_FORMAT_R32G32B32A32_FLOAT,1,16,D3D11_INPUT_PER_INSTANCE_DATA,1 },
	};

	if (m_pGridVertexShaderBlob == nullptr ||
		S_OK != m_pDevice->CreateInputLayout(gridIaDesc, 4, m_pGridVertexShaderBlob->GetBufferPointer(), m_pGridVertexShaderBlob->GetBufferSize(), &m_pipeline.pGridInputLayout))
		__debugbreak();

	m_pGridVertexShaderBlob->Release();
	m_pGridVertexShaderBlob = nullptr;

	D3D11_INPUT_ELEMENT_DESC spriteBatchIaDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "SPRITE",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,0,D3D11_INPUT_PER_INSTANCE_DATA,1 },
		{ "SPRITE",1,DXGI_FORMAT_R32G32B32A32_FLOAT,1,16,D3D11_INPUT_PER_INSTANCE_DATA,1 },
		{ "SPRITE",2,DXGI_FORMAT_R32G32B32A32_FLOAT,1,32,D3D11_INPUT_PER_INSTANCE_DATA,1 },
		{ "COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,48,D3D11_INPUT_PER_INSTANCE_DATA,1 },
	};

	if (m_pSpriteBatchVertexShaderBlob == nullptr ||
		S_OK != m_pDevice->CreateInputLayout(spriteBatchIaDesc, 6, m_pSpriteBatchVertexShaderBlob->GetBufferPointer(), m_pSpriteBatchVertexShaderBlob->GetBufferSize(), &m_pipeline.pSpriteBatchInputLayout))
		__debugbreak();

	m_pSpriteBatchVertexShaderBlob->Release();
	m_pSpriteBatchVertexShaderBlob = nullptr;

	D3D11_INPUT_ELEMENT_DESC glyphBatchIaDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0 },
		{ "GLYPH",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,0,D3D11_INPUT_PER_INSTANCE_DATA,1 },
		{ "ROWS",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,16,D3D11_INPUT_PER_INSTANCE_DATA,1 },
		{ "ROWS",1,DXGI_FORMAT_R32G32B32A32_FLOAT,1,32,D3D11_INPUT_PER_INSTANCE_DATA,1 },
		{ "COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,48,D3D11_INPUT_PER_INSTANCE_DATA,1 },
	};

	if (m_pGlyphBatchVertexShaderBlob == nullptr ||
		S_OK != m_pDevice->CreateInputLayout(glyphBatchIaDesc, 6, m_pGlyphBatchVertexShaderBlob->GetBufferPointer(), m_pGlyphBatchVertexShaderBlob->GetBufferSize(), &m_pipeline.pGlyphBatchInputLayout))
		__debugbreak();

	m_pGlyphBatchVertexShaderBlob->Release();
	m_pGlyphBatchVertexShaderBlob = nullptr;

	SpriteTransformData transformData{ XMMatrixIdentity(), m_matView, m_matProjection };
	SpriteData spriteData;
	m_pipeline.pTransformConstantBuffer = d3d::CreateConstantBuffer(m_pDevice, &transformData, sizeof(SpriteTransformData));
	m_pipeline.pSpriteConstantBuffer = d3d::CreateConstantBuffer(m_pDevice, &spriteData, sizeof(SpriteData));
}

void Renderer::ReleasePipeline()
{
	ReleaseFontAtlas();
	ReleaseTextures();
	if (m_pipeline.pPixelShader)
		m_pipeline.pPixelShader->Release();
	if (m_pipeline.pSpriteBatchPixelShader)
		m_pipeline.pSpriteBatchPixelShader->Release();
	if (m_pipeline.pGlyphBatchPixelShader)
		m_pipeline.pGlyphBatchPixelShader->Release();
	if (m_pipeline.pGlyphBatchVertexShader)
		m_pipeline.pGlyphBatchVertexShader->Release();
	if (m_pipeline.pSpriteBatchVertexShader)
		m_pipeline.pSpriteBatchVertexShader->Release();
	if (m_pipeline.pGridVertexShader)
		m_pipeline.pGridVertexShader->Release();
	if (m_pipeline.pVertexShader)
		m_pipeline.pVertexShader->Release();
	if (m_pipeline.pWhiteTexture)
		m_pipeline.pWhiteTexture->Release();
	if (m_pipeline.pGridInstanceBuffer)
		m_pipeline.pGridInstanceBuffer->Release();
	if (m_pipeline.pSpriteBatchInstanceBuffer)
		m_pipeline.pSpriteBatchInstanceBuffer->Release();
	if (m_pipeline.pGlyphBatchInstanceBuffer)
		m_pipeline.pGlyphBatchInstanceBuffer->Release();
	if (m_pipeline.pSpriteConstantBuffer)
		m_pipeline.pSpriteConstantBuffer->Release();
	if (m_pipeline.pTransformConstantBuffer)
		m_pipeline.pTransformConstantBuffer->Release();
	if (m_pGlyphBatchVertexShaderBlob)
		m_pGlyphBatchVertexShaderBlob->Release();
	if (m_pSpriteBatchVertexShaderBlob)
		m_pSpriteBatchVertexShaderBlob->Release();
	if (m_pGridVertexShaderBlob)
		m_pGridVertexShaderBlob->Release();
	if (m_pVertexShaderBlob)
		m_pVertexShaderBlob->Release();
	if (m_pipeline.pBlendState)
		m_pipeline.pBlendState->Release();
	if (m_pipeline.pDepthStencilState)
		m_pipeline.pDepthStencilState->Release();
	if (m_pipeline.pSampler)
		m_pipeline.pSampler->Release();
	if (m_pipeline.pInputLayout)
		m_pipeline.pInputLayout->Release();
	if (m_pipeline.pGridInputLayout)
		m_pipeline.pGridInputLayout->Release();
	if (m_pipeline.pSpriteBatchInputLayout)
		m_pipeline.pSpriteBatchInputLayout->Release();
	if (m_pipeline.pGlyphBatchInputLayout)
		m_pipeline.pGlyphBatchInputLayout->Release();
	if (m_pipeline.pRasterizer)
		m_pipeline.pRasterizer->Release();
	if (m_pipeline.pQuadIndexBuffer)
		m_pipeline.pQuadIndexBuffer->Release();
	if (m_pipeline.pQuadVertexBuffer)
		m_pipeline.pQuadVertexBuffer->Release();
}

void Renderer::ReleaseTextures()
{
	for (auto& texture : m_textureMap)
	{
		if (texture.second)
			texture.second->Release();
	}

	m_textureMap.clear();
	m_pipeline.pTexture = nullptr;
	m_lastTextureFile = nullptr;
	m_lastTextureView = nullptr;
}

void Renderer::ReleaseFontAtlas()
{
	if (m_fontAtlasView)
	{
		m_fontAtlasView->Release();
		m_fontAtlasView = nullptr;
	}
	if (m_fontAtlasTexture)
	{
		m_fontAtlasTexture->Release();
		m_fontAtlasTexture = nullptr;
	}
	if (m_fontDc != nullptr)
	{
		SelectObject(m_fontDc, GetStockObject(SYSTEM_FONT));
		DeleteDC(m_fontDc);
		m_fontDc = nullptr;
	}
	if (m_fontBitmap != nullptr)
	{
		DeleteObject(m_fontBitmap);
		m_fontBitmap = nullptr;
		m_fontBitmapPixels = nullptr;
	}
	if (m_fontHandle != nullptr)
	{
		DeleteObject(m_fontHandle);
		m_fontHandle = nullptr;
	}

	m_fontGlyphs.clear();
	m_fontAtlasPixels.clear();
	m_fontAtlasReady = false;
	m_fontAtlasDirty = false;
	m_fontGlyphCount = 0;
}

void Renderer::CreateWhiteTexture()
{
	const UINT whitePixel = 0xFFFFFFFF;

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = 1;
	textureDesc.Height = 1;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_IMMUTABLE;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA textureData = {};
	textureData.pSysMem = &whitePixel;
	textureData.SysMemPitch = sizeof(whitePixel);

	ID3D11Texture2D* texture = nullptr;
	if (S_OK != m_pDevice->CreateTexture2D(&textureDesc, &textureData, &texture))
		__debugbreak();

	if (S_OK != m_pDevice->CreateShaderResourceView(texture, nullptr, &m_pipeline.pWhiteTexture))
		__debugbreak();

	texture->Release();
}

bool Renderer::EnsureFontAtlas()
{
	if (m_fontAtlasReady)
		return true;

	WCHAR fontPath[MAX_PATH] = {};
	if (!ResolveWorkspaceFile(FontFileName, fontPath, MAX_PATH))
	{
		OutputDebugStringW(L"Font file was not found: ");
		OutputDebugStringW(FontFileName);
		OutputDebugStringW(L"\n");
		return false;
	}

	AddFontResourceExW(fontPath, FR_PRIVATE, nullptr);

	m_fontAtlasWidth = FontAtlasTextureSize;
	m_fontAtlasHeight = FontAtlasTextureSize;
	m_fontAtlasCellWidth = FontAtlasCellSize;
	m_fontAtlasCellHeight = FontAtlasCellSize;
	m_fontAtlasColumns = m_fontAtlasWidth / m_fontAtlasCellWidth;
	m_fontAtlasRows = m_fontAtlasHeight / m_fontAtlasCellHeight;
	m_fontGlyphCount = 0;
	m_fontPadding = 4;
	m_fontAtlasPixels.assign(static_cast<size_t>(m_fontAtlasWidth * m_fontAtlasHeight), 0);

	m_fontHandle = CreateFontW(
		-FontAtlasPixelHeight,
		0,
		0,
		0,
		FW_NORMAL,
		FALSE,
		FALSE,
		FALSE,
		DEFAULT_CHARSET,
		OUT_TT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		NONANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		FontFamilyName);
	if (m_fontHandle == nullptr)
		return false;

	m_fontDc = CreateCompatibleDC(nullptr);
	if (m_fontDc == nullptr)
		return false;

	BITMAPINFO bitmapInfo = {};
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = m_fontAtlasCellWidth;
	bitmapInfo.bmiHeader.biHeight = -m_fontAtlasCellHeight;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	m_fontBitmap = CreateDIBSection(m_fontDc, &bitmapInfo, DIB_RGB_COLORS, &m_fontBitmapPixels, nullptr, 0);
	if (m_fontBitmap == nullptr || m_fontBitmapPixels == nullptr)
		return false;

	SelectObject(m_fontDc, m_fontBitmap);
	SelectObject(m_fontDc, m_fontHandle);
	SetBkMode(m_fontDc, TRANSPARENT);
	SetTextColor(m_fontDc, RGB(255, 255, 255));

	SIZE asciiSize = {};
	GetTextExtentPoint32W(m_fontDc, L"A", 1, &asciiSize);
	m_fontAsciiWidth = static_cast<float>((std::max)(1L, asciiSize.cx));

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = static_cast<UINT>(m_fontAtlasWidth);
	textureDesc.Height = static_cast<UINT>(m_fontAtlasHeight);
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA textureData = {};
	textureData.pSysMem = m_fontAtlasPixels.data();
	textureData.SysMemPitch = static_cast<UINT>(m_fontAtlasWidth * sizeof(unsigned int));
	if (S_OK != m_pDevice->CreateTexture2D(&textureDesc, &textureData, &m_fontAtlasTexture))
		return false;
	if (S_OK != m_pDevice->CreateShaderResourceView(m_fontAtlasTexture, nullptr, &m_fontAtlasView))
		return false;

	m_fontAtlasReady = true;
	m_fontAtlasDirty = false;
	return true;
}

bool Renderer::EnsureFontGlyph(unsigned int codepoint)
{
	if (!EnsureFontAtlas())
		return false;

	if (m_fontGlyphs.find(codepoint) != m_fontGlyphs.end())
		return true;

	const int maxGlyphCount = m_fontAtlasColumns * m_fontAtlasRows;
	if (m_fontGlyphCount >= maxGlyphCount)
		return false;

	const int tileIndex = m_fontGlyphCount++;
	const int cellX = (tileIndex % m_fontAtlasColumns) * m_fontAtlasCellWidth;
	const int cellY = (tileIndex / m_fontAtlasColumns) * m_fontAtlasCellHeight;

	unsigned int* dibPixels = static_cast<unsigned int*>(m_fontBitmapPixels);
	std::fill(dibPixels, dibPixels + static_cast<size_t>(m_fontAtlasCellWidth * m_fontAtlasCellHeight), 0);

	wchar_t text[3] = {};
	int textLength = 1;
	if (codepoint <= 0xFFFF)
	{
		text[0] = static_cast<wchar_t>(codepoint);
	}
	else
	{
		const unsigned int adjusted = codepoint - 0x10000;
		text[0] = static_cast<wchar_t>(0xD800 + (adjusted >> 10));
		text[1] = static_cast<wchar_t>(0xDC00 + (adjusted & 0x3FF));
		textLength = 2;
	}

	SIZE glyphSize = {};
	GetTextExtentPoint32W(m_fontDc, text, textLength, &glyphSize);
	const int drawX = m_fontPadding;
	const int drawY = m_fontPadding;
	TextOutW(m_fontDc, drawX, drawY, text, textLength);

	for (int y = 0; y < m_fontAtlasCellHeight; ++y)
	{
		for (int x = 0; x < m_fontAtlasCellWidth; ++x)
		{
			const unsigned int source = dibPixels[y * m_fontAtlasCellWidth + x];
			const unsigned int red = source & 0x000000FFu;
			const unsigned int green = (source >> 8) & 0x000000FFu;
			const unsigned int blue = (source >> 16) & 0x000000FFu;
			const unsigned int alpha = (std::max)(red, (std::max)(green, blue));
			const unsigned int target = alpha == 0 ? 0 : ((alpha << 24) | 0x00FFFFFFu);
			m_fontAtlasPixels[(cellY + y) * m_fontAtlasWidth + (cellX + x)] = target;
		}
	}

	FontGlyph glyph;
	glyph.tileIndex = tileIndex;
	glyph.widthPixels = static_cast<float>((std::max)(1L, glyphSize.cx));
	glyph.advancePixels = glyph.widthPixels;
	m_fontGlyphs.insert({ codepoint, glyph });
	m_fontAtlasDirty = true;
	return true;
}

void Renderer::UploadFontAtlas()
{
	if (!m_fontAtlasReady || !m_fontAtlasDirty || m_fontAtlasTexture == nullptr || m_fontAtlasPixels.empty())
		return;

	ID3D11ShaderResourceView* nullView = nullptr;
	m_pDeviceContext->PSSetShaderResources(0, 1, &nullView);
	if (m_boundTexture == m_fontAtlasView)
		m_boundTexture = nullptr;

	m_pDeviceContext->UpdateSubresource(
		m_fontAtlasTexture,
		0,
		nullptr,
		m_fontAtlasPixels.data(),
		static_cast<UINT>(m_fontAtlasWidth * sizeof(unsigned int)),
		0);
	const unsigned int uploadBytes = static_cast<unsigned int>(m_fontAtlasPixels.size() * sizeof(unsigned int));
	++m_currentFrameStats.fontAtlasUploads;
	m_currentFrameStats.fontAtlasUploadBytes += uploadBytes;
	m_fontAtlasDirty = false;
}

unsigned int Renderer::DecodeUtf8Codepoint(const char*& cursor) const
{
	if (cursor == nullptr || *cursor == '\0')
		return 0;

	const unsigned char first = static_cast<unsigned char>(*cursor++);
	if (first < 0x80)
		return first;

	if ((first & 0xE0) == 0xC0)
	{
		const unsigned char second = static_cast<unsigned char>(*cursor);
		if ((second & 0xC0) != 0x80)
			return '?';
		++cursor;
		return ((first & 0x1Fu) << 6) | (second & 0x3Fu);
	}

	if ((first & 0xF0) == 0xE0)
	{
		const unsigned char second = static_cast<unsigned char>(*cursor);
		const unsigned char third = static_cast<unsigned char>(*(cursor + 1));
		if ((second & 0xC0) != 0x80 || (third & 0xC0) != 0x80)
			return '?';
		cursor += 2;
		return ((first & 0x0Fu) << 12) | ((second & 0x3Fu) << 6) | (third & 0x3Fu);
	}

	if ((first & 0xF8) == 0xF0)
	{
		const unsigned char second = static_cast<unsigned char>(*cursor);
		const unsigned char third = static_cast<unsigned char>(*(cursor + 1));
		const unsigned char fourth = static_cast<unsigned char>(*(cursor + 2));
		if ((second & 0xC0) != 0x80 || (third & 0xC0) != 0x80 || (fourth & 0xC0) != 0x80)
			return '?';
		cursor += 3;
		return ((first & 0x07u) << 18) | ((second & 0x3Fu) << 12) | ((third & 0x3Fu) << 6) | (fourth & 0x3Fu);
	}

	return '?';
}

void Renderer::BindSpritePipeline()
{
	if (m_spritePipelineBound)
		return;

	m_pDeviceContext->IASetInputLayout(m_pipeline.pInputLayout);
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->VSSetShader(m_pipeline.pVertexShader, nullptr, 0);
	m_pDeviceContext->PSSetShader(m_pipeline.pPixelShader, nullptr, 0);
	m_pDeviceContext->PSSetSamplers(0, 1, &m_pipeline.pSampler);
	m_pDeviceContext->RSSetState(m_pipeline.pRasterizer);
	m_pDeviceContext->OMSetBlendState(m_pipeline.pBlendState, nullptr, 0xFFFFFFFF);
	m_pDeviceContext->OMSetDepthStencilState(m_pipeline.pDepthStencilState, 0);

	m_spritePipelineBound = true;
}

void Renderer::BindTexture(ID3D11ShaderResourceView* texture)
{
	if (m_boundTexture == texture)
		return;

	m_pDeviceContext->PSSetShaderResources(0, 1, &texture);
	m_boundTexture = texture;
	++m_currentFrameStats.textureBinds;
}

void Renderer::EnsureGridBatchCapacity(size_t quadCount)
{
	if (quadCount == 0 || quadCount <= m_gridQuadCapacity)
		return;

	if (m_pipeline.pGridInstanceBuffer)
	{
		m_pipeline.pGridInstanceBuffer->Release();
		m_pipeline.pGridInstanceBuffer = nullptr;
	}

	const size_t doubledCapacity = m_gridQuadCapacity > 0 ? m_gridQuadCapacity * 2 : 256;
	m_gridQuadCapacity = quadCount > doubledCapacity ? quadCount : doubledCapacity;

	m_pipeline.pGridInstanceBuffer = CreateDynamicBuffer(m_pDevice,
		static_cast<UINT>(m_gridQuadCapacity * sizeof(GridInstance)),
		D3D11_BIND_VERTEX_BUFFER);
}

void Renderer::EnsureSpriteBatchCapacity(size_t quadCount)
{
	if (quadCount == 0 || quadCount <= m_spriteBatchQuadCapacity)
		return;

	if (m_pipeline.pSpriteBatchInstanceBuffer)
	{
		m_pipeline.pSpriteBatchInstanceBuffer->Release();
		m_pipeline.pSpriteBatchInstanceBuffer = nullptr;
	}

	const size_t doubledCapacity = m_spriteBatchQuadCapacity > 0 ? m_spriteBatchQuadCapacity * 2 : 512;
	m_spriteBatchQuadCapacity = quadCount > doubledCapacity ? quadCount : doubledCapacity;

	m_pipeline.pSpriteBatchInstanceBuffer = CreateDynamicBuffer(m_pDevice,
		static_cast<UINT>(m_spriteBatchQuadCapacity * sizeof(SpriteBatchInstance)),
		D3D11_BIND_VERTEX_BUFFER);
}

void Renderer::EnsureGlyphBatchCapacity(size_t quadCount)
{
	if (quadCount == 0 || quadCount <= m_glyphBatchQuadCapacity)
		return;

	if (m_pipeline.pGlyphBatchInstanceBuffer)
	{
		m_pipeline.pGlyphBatchInstanceBuffer->Release();
		m_pipeline.pGlyphBatchInstanceBuffer = nullptr;
	}

	const size_t doubledCapacity = m_glyphBatchQuadCapacity > 0 ? m_glyphBatchQuadCapacity * 2 : 512;
	m_glyphBatchQuadCapacity = quadCount > doubledCapacity ? quadCount : doubledCapacity;

	m_pipeline.pGlyphBatchInstanceBuffer = CreateDynamicBuffer(m_pDevice,
		static_cast<UINT>(m_glyphBatchQuadCapacity * sizeof(GlyphBatchInstance)),
		D3D11_BIND_VERTEX_BUFFER);
}

void Renderer::ResetGridCache()
{
	m_gridCache = GridCacheState();
}

void Renderer::RebuildGridChunk(const BlockGridDesc& desc, GridChunkCache& chunk, int chunkX, int chunkY,
	int chunkSize, int atlasColumns, int atlasRows, int tileCount, float uvWidth, float uvHeight)
{
	const int startX = (std::max)(0, chunkX * chunkSize);
	const int endX = (std::min)(desc.width - 1, startX + chunkSize - 1);
	const int startY = (std::max)(0, chunkY * chunkSize);
	const int endY = (std::min)(desc.height - 1, startY + chunkSize - 1);
	chunk.startX = startX;
	chunk.startY = startY;
	chunk.endX = endX;
	chunk.endY = endY;
	chunk.instances.clear();
	chunk.instances.reserve(static_cast<size_t>((endX - startX + 1) * (endY - startY + 1)));

	for (int y = startY; y <= endY; ++y)
	{
		const BlockTile* row = desc.tiles + y * desc.width;
		for (int x = startX; x <= endX; ++x)
		{
			const BlockTile& tile = row[x];
			if (!tile.visible)
				continue;

			int tileIndex = tile.tileIndex;
			if (desc.atlasTileRemap != nullptr && tileIndex >= 0 && tileIndex < desc.atlasTileRemapCount)
				tileIndex = desc.atlasTileRemap[tileIndex];
			if (tileCount > 0)
				tileIndex = tileIndex % tileCount;

			const int tileX = tileIndex % atlasColumns;
			const int tileY = tileIndex / atlasColumns;
			const float u0 = tileX * uvWidth;
			const float v0 = tileY * uvHeight;
			chunk.instances.push_back({
				math::Vec4(static_cast<float>(x) * desc.tileSize, -static_cast<float>(y) * desc.tileSize, desc.tileSize, 0.0f),
				math::Vec4(u0, v0, uvWidth, uvHeight) });
		}
	}

	chunk.valid = true;
}

SpriteBatchInstance Renderer::BuildSpriteBatchInstance(const SpriteDesc& desc) const
{
	int atlasColumns = desc.atlasColumns > 0 ? desc.atlasColumns : 1;
	int atlasRows = desc.atlasRows > 0 ? desc.atlasRows : 1;
	int tileCount = atlasColumns * atlasRows;
	int tileIndex = desc.tileIndex;
	if (tileIndex < 0)
		tileIndex = 0;
	if (tileCount > 0)
		tileIndex = tileIndex % tileCount;

	const int tileX = tileIndex % atlasColumns;
	const int tileY = tileIndex / atlasColumns;
	const float uvWidth = 1.0f / atlasColumns;
	const float uvHeight = 1.0f / atlasRows;
	float u0 = tileX * uvWidth;
	float widthScale = uvWidth;
	if (desc.flipX)
	{
		u0 = (tileX + 1) * uvWidth;
		widthScale = -uvWidth;
	}

	SpriteBatchInstance instance;
	instance.transform = { desc.positionX, desc.positionY, desc.width, desc.height };
	instance.rotationDepth = { std::sin(desc.rotationRadians), std::cos(desc.rotationRadians), desc.depth, 0.0f };
	instance.uv = { u0, tileY * uvHeight, widthScale, uvHeight };
	instance.color = { desc.colorR, desc.colorG, desc.colorB, desc.colorA };
	return instance;
}

void Renderer::QueueSprite(ID3D11ShaderResourceView* texture, const SpriteDesc& desc)
{
	if (texture == nullptr)
		return;

	if (m_spriteBatchTexture != nullptr && m_spriteBatchTexture != texture)
		FlushSpriteBatch();

	m_spriteBatchTexture = texture;
	m_spriteBatchInstances.push_back(BuildSpriteBatchInstance(desc));
	if (texture == m_pipeline.pWhiteTexture)
		++m_currentFrameStats.whiteQuads;
	else
		++m_currentFrameStats.texturedQuads;
}

void Renderer::FlushSpriteBatch()
{
	if (m_spriteBatchInstances.empty())
		return;

	if (m_pipeline.pSpriteBatchVertexShader == nullptr || m_pipeline.pSpriteBatchPixelShader == nullptr ||
		m_pipeline.pSpriteBatchInputLayout == nullptr || m_spriteBatchTexture == nullptr)
	{
		m_spriteBatchInstances.clear();
		m_spriteBatchTexture = nullptr;
		return;
	}

	const size_t instanceCount = m_spriteBatchInstances.size();
	EnsureSpriteBatchCapacity(instanceCount);
	const unsigned int uploadBytes = static_cast<unsigned int>(instanceCount * sizeof(SpriteBatchInstance));

	D3D11_MAPPED_SUBRESOURCE instanceResource;
	if (S_OK != m_pDeviceContext->Map(m_pipeline.pSpriteBatchInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &instanceResource))
		__debugbreak();
	memcpy_s(instanceResource.pData, m_spriteBatchQuadCapacity * sizeof(SpriteBatchInstance),
		m_spriteBatchInstances.data(), m_spriteBatchInstances.size() * sizeof(SpriteBatchInstance));
	m_pDeviceContext->Unmap(m_pipeline.pSpriteBatchInstanceBuffer, 0);

	ID3D11Buffer* vertexBuffers[] = { m_pipeline.pQuadVertexBuffer, m_pipeline.pSpriteBatchInstanceBuffer };
	UINT strides[] = { sizeof(SimpleVertex), sizeof(SpriteBatchInstance) };
	UINT offsets[] = { 0, 0 };

	m_spritePipelineBound = false;
	m_pDeviceContext->IASetInputLayout(m_pipeline.pSpriteBatchInputLayout);
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->VSSetShader(m_pipeline.pSpriteBatchVertexShader, nullptr, 0);
	m_pDeviceContext->PSSetShader(m_pipeline.pSpriteBatchPixelShader, nullptr, 0);
	m_pDeviceContext->PSSetSamplers(0, 1, &m_pipeline.pSampler);
	m_pDeviceContext->RSSetState(m_pipeline.pRasterizer);
	m_pDeviceContext->OMSetBlendState(m_pipeline.pBlendState, nullptr, 0xFFFFFFFF);
	m_pDeviceContext->OMSetDepthStencilState(m_pipeline.pDepthStencilState, 0);
	BindTexture(m_spriteBatchTexture);
	m_pDeviceContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
	m_pDeviceContext->IASetIndexBuffer(m_pipeline.pQuadIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	SpriteTransformData transformData{ XMMatrixIdentity(), m_matView, m_matProjection };
	d3d::BindVertexConstantBuffer(m_pDeviceContext, m_pipeline.pTransformConstantBuffer, &transformData, sizeof(SpriteTransformData), 0);
	m_pDeviceContext->DrawIndexedInstanced(6, static_cast<UINT>(instanceCount), 0, 0, 0);

	++m_currentFrameStats.drawCalls;
	++m_currentFrameStats.spriteDrawCalls;
	m_currentFrameStats.spriteQuads += static_cast<unsigned int>(instanceCount);
	++m_currentFrameStats.spriteBatches;
	m_currentFrameStats.maxSpriteBatchQuads = (std::max)(m_currentFrameStats.maxSpriteBatchQuads, static_cast<unsigned int>(instanceCount));
	++m_currentFrameStats.dynamicBufferUploads;
	m_currentFrameStats.dynamicBufferUploadBytes += uploadBytes;
	m_currentFrameStats.spriteUploadBytes += uploadBytes;
	m_spriteBatchInstances.clear();
	m_spriteBatchTexture = nullptr;
	m_spritePipelineBound = false;
}

void Renderer::FlushFontSpriteBatch()
{
	if (m_fontSpriteBatchInstances.empty())
		return;

	if (m_pipeline.pSpriteBatchVertexShader == nullptr || m_pipeline.pSpriteBatchPixelShader == nullptr ||
		m_pipeline.pSpriteBatchInputLayout == nullptr || m_fontAtlasView == nullptr)
	{
		m_fontSpriteBatchInstances.clear();
		return;
	}

	const size_t instanceCount = m_fontSpriteBatchInstances.size();
	EnsureSpriteBatchCapacity(instanceCount);
	const unsigned int uploadBytes = static_cast<unsigned int>(instanceCount * sizeof(SpriteBatchInstance));

	D3D11_MAPPED_SUBRESOURCE instanceResource;
	if (S_OK != m_pDeviceContext->Map(m_pipeline.pSpriteBatchInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &instanceResource))
		__debugbreak();
	memcpy_s(instanceResource.pData, m_spriteBatchQuadCapacity * sizeof(SpriteBatchInstance),
		m_fontSpriteBatchInstances.data(), m_fontSpriteBatchInstances.size() * sizeof(SpriteBatchInstance));
	m_pDeviceContext->Unmap(m_pipeline.pSpriteBatchInstanceBuffer, 0);

	ID3D11Buffer* vertexBuffers[] = { m_pipeline.pQuadVertexBuffer, m_pipeline.pSpriteBatchInstanceBuffer };
	UINT strides[] = { sizeof(SimpleVertex), sizeof(SpriteBatchInstance) };
	UINT offsets[] = { 0, 0 };

	m_spritePipelineBound = false;
	m_pDeviceContext->IASetInputLayout(m_pipeline.pSpriteBatchInputLayout);
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->VSSetShader(m_pipeline.pSpriteBatchVertexShader, nullptr, 0);
	m_pDeviceContext->PSSetShader(m_pipeline.pSpriteBatchPixelShader, nullptr, 0);
	m_pDeviceContext->PSSetSamplers(0, 1, &m_pipeline.pSampler);
	m_pDeviceContext->RSSetState(m_pipeline.pRasterizer);
	m_pDeviceContext->OMSetBlendState(m_pipeline.pBlendState, nullptr, 0xFFFFFFFF);
	m_pDeviceContext->OMSetDepthStencilState(m_pipeline.pDepthStencilState, 0);
	BindTexture(m_fontAtlasView);
	m_pDeviceContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
	m_pDeviceContext->IASetIndexBuffer(m_pipeline.pQuadIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	SpriteTransformData transformData{ XMMatrixIdentity(), m_matView, m_matProjection };
	d3d::BindVertexConstantBuffer(m_pDeviceContext, m_pipeline.pTransformConstantBuffer, &transformData, sizeof(SpriteTransformData), 0);
	m_pDeviceContext->DrawIndexedInstanced(6, static_cast<UINT>(instanceCount), 0, 0, 0);

	++m_currentFrameStats.drawCalls;
	++m_currentFrameStats.fontDrawCalls;
	m_currentFrameStats.fontQuads += static_cast<unsigned int>(instanceCount);
	++m_currentFrameStats.fontBatches;
	m_currentFrameStats.maxFontBatchQuads = (std::max)(m_currentFrameStats.maxFontBatchQuads, static_cast<unsigned int>(instanceCount));
	++m_currentFrameStats.dynamicBufferUploads;
	m_currentFrameStats.dynamicBufferUploadBytes += uploadBytes;
	m_currentFrameStats.fontUploadBytes += uploadBytes;
	m_fontSpriteBatchInstances.clear();
	m_spritePipelineBound = false;
}

void Renderer::FlushGlyphBatch()
{
	if (m_glyphBatchInstances.empty())
		return;

	if (m_pipeline.pGlyphBatchVertexShader == nullptr || m_pipeline.pGlyphBatchPixelShader == nullptr ||
		m_pipeline.pGlyphBatchInputLayout == nullptr)
	{
		m_glyphBatchInstances.clear();
		return;
	}

	const size_t instanceCount = m_glyphBatchInstances.size();
	EnsureGlyphBatchCapacity(instanceCount);
	const unsigned int uploadBytes = static_cast<unsigned int>(instanceCount * sizeof(GlyphBatchInstance));

	D3D11_MAPPED_SUBRESOURCE instanceResource;
	if (S_OK != m_pDeviceContext->Map(m_pipeline.pGlyphBatchInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &instanceResource))
		__debugbreak();
	memcpy_s(instanceResource.pData, m_glyphBatchQuadCapacity * sizeof(GlyphBatchInstance),
		m_glyphBatchInstances.data(), m_glyphBatchInstances.size() * sizeof(GlyphBatchInstance));
	m_pDeviceContext->Unmap(m_pipeline.pGlyphBatchInstanceBuffer, 0);

	ID3D11Buffer* vertexBuffers[] = { m_pipeline.pQuadVertexBuffer, m_pipeline.pGlyphBatchInstanceBuffer };
	UINT strides[] = { sizeof(SimpleVertex), sizeof(GlyphBatchInstance) };
	UINT offsets[] = { 0, 0 };

	m_spritePipelineBound = false;
	m_pDeviceContext->IASetInputLayout(m_pipeline.pGlyphBatchInputLayout);
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->VSSetShader(m_pipeline.pGlyphBatchVertexShader, nullptr, 0);
	m_pDeviceContext->PSSetShader(m_pipeline.pGlyphBatchPixelShader, nullptr, 0);
	m_pDeviceContext->RSSetState(m_pipeline.pRasterizer);
	m_pDeviceContext->OMSetBlendState(m_pipeline.pBlendState, nullptr, 0xFFFFFFFF);
	m_pDeviceContext->OMSetDepthStencilState(m_pipeline.pDepthStencilState, 0);
	m_pDeviceContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
	m_pDeviceContext->IASetIndexBuffer(m_pipeline.pQuadIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	SpriteTransformData transformData{ XMMatrixIdentity(), m_matView, m_matProjection };
	d3d::BindVertexConstantBuffer(m_pDeviceContext, m_pipeline.pTransformConstantBuffer, &transformData, sizeof(SpriteTransformData), 0);
	m_pDeviceContext->DrawIndexedInstanced(6, static_cast<UINT>(instanceCount), 0, 0, 0);

	++m_currentFrameStats.drawCalls;
	++m_currentFrameStats.glyphDrawCalls;
	m_currentFrameStats.glyphQuads += static_cast<unsigned int>(instanceCount);
	++m_currentFrameStats.glyphBatches;
	m_currentFrameStats.maxGlyphBatchQuads = (std::max)(m_currentFrameStats.maxGlyphBatchQuads, static_cast<unsigned int>(instanceCount));
	++m_currentFrameStats.dynamicBufferUploads;
	m_currentFrameStats.dynamicBufferUploadBytes += uploadBytes;
	m_currentFrameStats.glyphUploadBytes += uploadBytes;
	m_glyphBatchInstances.clear();
	m_spritePipelineBound = false;
}

void Renderer::LoadTexture(const WCHAR* textureFile)
{
	m_pipeline.pTexture = GetTexture(textureFile);
}

ID3D11ShaderResourceView* Renderer::GetTexture(const WCHAR* textureFile)
{
	if (textureFile == nullptr)
		return nullptr;

	if (m_lastTextureFile == textureFile && m_lastTextureView != nullptr)
		return m_lastTextureView;

	std::wstring key(textureFile);
	auto iter = m_textureMap.find(key);
	if (iter != m_textureMap.end())
	{
		m_lastTextureFile = textureFile;
		m_lastTextureView = iter->second;
		return iter->second;
	}

	WCHAR wszFullPath[MAX_PATH] = {};
	if (!ResolveWorkspaceFile(textureFile, wszFullPath, MAX_PATH))
	{
		OutputDebugStringW(L"Texture file was not found: ");
		OutputDebugStringW(textureFile);
		OutputDebugStringW(L"\n");
		__debugbreak();
		return nullptr;
	}

	WCHAR* wszExt = PathFindExtensionW(textureFile);

	ID3D11ShaderResourceView* pSRV = nullptr;
	TexMetadata metaData;
	ScratchImage scratchImage;
	HRESULT hr = E_FAIL;

	if (wcscmp(wszExt, L".dds") == 0)
	{
		hr = DirectX::LoadFromDDSFile(wszFullPath, DirectX::DDS_FLAGS_NONE, &metaData, scratchImage);
	}
	else if (wcscmp(wszExt, L".png") == 0 || wcscmp(wszExt, L".jpg") == 0 || wcscmp(wszExt, L".jpeg") == 0 || wcscmp(wszExt, L".gif") == 0 || wcscmp(wszExt, L".bmp") == 0)
	{
		hr = DirectX::LoadFromWICFile(wszFullPath, DirectX::WIC_FLAGS_NONE, &metaData, scratchImage);
	}
	else if (wcscmp(wszExt, L".tga") == 0)
	{
		hr = DirectX::LoadFromTGAFile(wszFullPath, &metaData, scratchImage);
	}

	if (FAILED(hr))
	{
		OutputDebugStringW(L"Texture load failed: ");
		OutputDebugStringW(wszFullPath);
		OutputDebugStringW(L"\n");
		__debugbreak();
		return nullptr;
	}

	hr = DirectX::CreateShaderResourceView(m_pDevice, scratchImage.GetImages(), scratchImage.GetImageCount(), metaData, &pSRV);
	if (FAILED(hr))
	{
		OutputDebugStringW(L"Texture shader resource view creation failed: ");
		OutputDebugStringW(wszFullPath);
		OutputDebugStringW(L"\n");
		__debugbreak();
		return nullptr;
	}

	m_textureMap.insert({ key, pSRV });
	m_lastTextureFile = textureFile;
	m_lastTextureView = pSRV;
	return pSRV;
}

void Renderer::LoadPipelineShader(const WCHAR* shaderFile)
{
	WCHAR shaderPath[MAX_PATH] = {};
	if (!ResolveWorkspaceFile(shaderFile, shaderPath, MAX_PATH))
	{
		OutputDebugStringW(L"Shader file was not found: ");
		OutputDebugStringW(shaderFile);
		OutputDebugStringW(L"\n");
		__debugbreak();
		return;
	}

	char shaderBaseName[ShaderEntryNameCapacity] = {};
	if (!GetShaderBaseName(shaderFile, shaderBaseName, ShaderEntryNameCapacity))
	{
		OutputDebugStringW(L"Shader file name was invalid: ");
		OutputDebugStringW(shaderFile);
		OutputDebugStringW(L"\n");
		__debugbreak();
		return;
	}

	int flag = 0;
#ifdef _DEBUG
	flag = D3DCOMPILE_DEBUG;
#endif
	flag |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

	auto compileVertexShader = [&](const char* suffix, ID3D11VertexShader** shader) -> ID3DBlob*
	{
		char entryName[ShaderEntryNameCapacity] = {};
		BuildShaderEntryName(entryName, ShaderEntryNameCapacity, shaderBaseName, suffix);

		ID3DBlob* shaderBlob = CompileShaderEntry(shaderPath, entryName, "vs_5_0", flag);
		if (shaderBlob == nullptr)
			return nullptr;

		if (S_OK != m_pDevice->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, shader))
		{
			shaderBlob->Release();
			__debugbreak();
			return nullptr;
		}

		return shaderBlob;
	};

	auto compilePixelShader = [&](const char* suffix, ID3D11PixelShader** shader)
	{
		char entryName[ShaderEntryNameCapacity] = {};
		BuildShaderEntryName(entryName, ShaderEntryNameCapacity, shaderBaseName, suffix);

		ID3DBlob* shaderBlob = CompileShaderEntry(shaderPath, entryName, "ps_5_0", flag);
		if (shaderBlob == nullptr)
			return;

		if (S_OK != m_pDevice->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, shader))
			__debugbreak();

		shaderBlob->Release();
	};

	m_pVertexShaderBlob = compileVertexShader("_VS", &m_pipeline.pVertexShader);
	m_pGridVertexShaderBlob = compileVertexShader("_GridVS", &m_pipeline.pGridVertexShader);
	m_pSpriteBatchVertexShaderBlob = compileVertexShader("_BatchVS", &m_pipeline.pSpriteBatchVertexShader);
	compilePixelShader("_BatchPS", &m_pipeline.pSpriteBatchPixelShader);
	m_pGlyphBatchVertexShaderBlob = compileVertexShader("_GlyphVS", &m_pipeline.pGlyphBatchVertexShader);
	compilePixelShader("_GlyphPS", &m_pipeline.pGlyphBatchPixelShader);
	compilePixelShader("_PS", &m_pipeline.pPixelShader);
}

void Renderer::DrawBlockGrid(const BlockGridDesc& desc)
{
	if (desc.textureFile == nullptr || desc.tiles == nullptr || desc.width <= 0 || desc.height <= 0 || desc.tileSize <= 0.0f)
		return;

	ID3D11ShaderResourceView* texture = GetTexture(desc.textureFile);
	if (m_pipeline.pGridVertexShader == nullptr || m_pipeline.pGridInputLayout == nullptr ||
		m_pipeline.pPixelShader == nullptr || texture == nullptr)
		return;

	const int atlasColumns = desc.atlasColumns > 0 ? desc.atlasColumns : 1;
	const int atlasRows = desc.atlasRows > 0 ? desc.atlasRows : 1;
	const int tileCount = atlasColumns * atlasRows;
	const float uvWidth = 1.0f / atlasColumns;
	const float uvHeight = 1.0f / atlasRows;
	const float halfTile = desc.tileSize * 0.5f;
	const float minVisibleX = -m_viewHalfWidth - halfTile;
	const float maxVisibleX = m_viewHalfWidth + halfTile;
	const float minVisibleY = -m_viewHalfHeight - halfTile;
	const float maxVisibleY = m_viewHalfHeight + halfTile;
	const int startX = (std::max)(0, static_cast<int>(std::floor((minVisibleX - desc.originX) / desc.tileSize)));
	const int endX = (std::min)(desc.width - 1, static_cast<int>(std::ceil((maxVisibleX - desc.originX) / desc.tileSize)));
	const int startY = (std::max)(0, static_cast<int>(std::floor((desc.originY - maxVisibleY) / desc.tileSize)));
	const int endY = (std::min)(desc.height - 1, static_cast<int>(std::ceil((desc.originY - minVisibleY) / desc.tileSize)));
	if (startX > endX || startY > endY)
		return;

	m_currentFrameStats.gridVisibleColumns = static_cast<unsigned int>(endX - startX + 1);
	m_currentFrameStats.gridVisibleRows = static_cast<unsigned int>(endY - startY + 1);
	m_currentFrameStats.gridVisibleTiles = m_currentFrameStats.gridVisibleColumns * m_currentFrameStats.gridVisibleRows;

	FlushSpriteBatch();
	FlushFontSpriteBatch();

	m_gridInstances.clear();
	XMMATRIX gridWorld = XMMatrixIdentity();
	const bool useChunkCache = desc.chunkVersions != nullptr && desc.chunkSizeTiles > 0;
	size_t instanceCount = 0;
	bool gridBufferUploaded = false;

	if (useChunkCache)
	{
		const int chunkSize = (std::max)(1, desc.chunkSizeTiles);
		const int chunkColumns = desc.chunkColumns > 0 ? desc.chunkColumns : (desc.width + chunkSize - 1) / chunkSize;
		const int chunkRows = desc.chunkRows > 0 ? desc.chunkRows : (desc.height + chunkSize - 1) / chunkSize;
		const bool cacheMismatch =
			m_gridCache.tiles != desc.tiles ||
			m_gridCache.width != desc.width ||
			m_gridCache.height != desc.height ||
			m_gridCache.atlasColumns != atlasColumns ||
			m_gridCache.atlasRows != atlasRows ||
			m_gridCache.chunkSizeTiles != chunkSize ||
			m_gridCache.chunkColumns != chunkColumns ||
			m_gridCache.chunkRows != chunkRows ||
			m_gridCache.gridVersion != desc.gridVersion ||
			m_gridCache.tileSize != desc.tileSize;

		if (cacheMismatch)
		{
			++m_currentFrameStats.gridCacheInvalidations;
			m_gridCache.tiles = desc.tiles;
			m_gridCache.width = desc.width;
			m_gridCache.height = desc.height;
			m_gridCache.atlasColumns = atlasColumns;
			m_gridCache.atlasRows = atlasRows;
			m_gridCache.chunkSizeTiles = chunkSize;
			m_gridCache.chunkColumns = chunkColumns;
			m_gridCache.chunkRows = chunkRows;
			m_gridCache.gridVersion = desc.gridVersion;
			m_gridCache.tileSize = desc.tileSize;
			m_gridCache.chunks.assign(static_cast<size_t>(chunkColumns * chunkRows), GridChunkCache());
		}

		const int minChunkX = std::clamp(startX / chunkSize, 0, chunkColumns - 1);
		const int maxChunkX = std::clamp(endX / chunkSize, 0, chunkColumns - 1);
		const int minChunkY = std::clamp(startY / chunkSize, 0, chunkRows - 1);
		const int maxChunkY = std::clamp(endY / chunkSize, 0, chunkRows - 1);
		instanceCount = 0;

		for (int chunkY = minChunkY; chunkY <= maxChunkY; ++chunkY)
		{
			for (int chunkX = minChunkX; chunkX <= maxChunkX; ++chunkX)
			{
				const int chunkIndex = chunkY * chunkColumns + chunkX;
				GridChunkCache& chunk = m_gridCache.chunks[chunkIndex];
				const unsigned int version = desc.chunkVersions[chunkIndex];
				if (!chunk.valid || chunk.version != version)
				{
					RebuildGridChunk(desc, chunk, chunkX, chunkY, chunkSize, atlasColumns, atlasRows, tileCount, uvWidth, uvHeight);
					chunk.version = version;
					++m_currentFrameStats.gridChunksRebuilt;
				}

				instanceCount += chunk.instances.size();
				++m_currentFrameStats.gridChunksDrawn;
			}
		}

		if (instanceCount == 0)
			return;

		EnsureGridBatchCapacity(instanceCount);
		const unsigned int uploadBytes = static_cast<unsigned int>(instanceCount * sizeof(GridInstance));
		D3D11_MAPPED_SUBRESOURCE instanceResource;
		if (S_OK != m_pDeviceContext->Map(m_pipeline.pGridInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &instanceResource))
			__debugbreak();

		GridInstance* writeCursor = static_cast<GridInstance*>(instanceResource.pData);
		size_t copiedInstances = 0;
		for (int chunkY = minChunkY; chunkY <= maxChunkY; ++chunkY)
		{
			for (int chunkX = minChunkX; chunkX <= maxChunkX; ++chunkX)
			{
				const int chunkIndex = chunkY * chunkColumns + chunkX;
				const std::vector<GridInstance>& instances = m_gridCache.chunks[chunkIndex].instances;
				if (instances.empty())
					continue;

				memcpy_s(writeCursor + copiedInstances,
					(m_gridQuadCapacity - copiedInstances) * sizeof(GridInstance),
					instances.data(),
					instances.size() * sizeof(GridInstance));
				copiedInstances += instances.size();
			}
		}
		m_pDeviceContext->Unmap(m_pipeline.pGridInstanceBuffer, 0);
		++m_currentFrameStats.dynamicBufferUploads;
		m_currentFrameStats.dynamicBufferUploadBytes += uploadBytes;
		m_currentFrameStats.gridUploadBytes += uploadBytes;

		gridBufferUploaded = true;
		gridWorld = XMMatrixTranslation(desc.originX, desc.originY, 0.0f);
	}
	else
	{
		const int visibleTileCapacity = (endX - startX + 1) * (endY - startY + 1);
		m_gridInstances.reserve(visibleTileCapacity);

		for (int y = startY; y <= endY; ++y)
		{
			const BlockTile* row = desc.tiles + y * desc.width;
			for (int x = startX; x <= endX; ++x)
			{
				const BlockTile& tile = row[x];
				if (!tile.visible)
					continue;

				const float centerX = desc.originX + x * desc.tileSize;
				const float centerY = desc.originY - y * desc.tileSize;

				int tileIndex = tile.tileIndex;
				if (desc.atlasTileRemap != nullptr && tileIndex >= 0 && tileIndex < desc.atlasTileRemapCount)
					tileIndex = desc.atlasTileRemap[tileIndex];
				if (tileCount > 0)
					tileIndex = tileIndex % tileCount;

				const int tileX = tileIndex % atlasColumns;
				const int tileY = tileIndex / atlasColumns;
				const float u0 = tileX * uvWidth;
				const float v0 = tileY * uvHeight;
				m_gridInstances.push_back({ math::Vec4(centerX, centerY, desc.tileSize, 0.0f), math::Vec4(u0, v0, uvWidth, uvHeight) });
			}
		}
	}

	if (!gridBufferUploaded)
	{
		if (m_gridInstances.empty())
			return;

		instanceCount = m_gridInstances.size();
		EnsureGridBatchCapacity(instanceCount);
		const unsigned int uploadBytes = static_cast<unsigned int>(instanceCount * sizeof(GridInstance));

		D3D11_MAPPED_SUBRESOURCE instanceResource;
		if (S_OK != m_pDeviceContext->Map(m_pipeline.pGridInstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &instanceResource))
			__debugbreak();
		memcpy_s(instanceResource.pData, m_gridQuadCapacity * sizeof(GridInstance),
			m_gridInstances.data(), m_gridInstances.size() * sizeof(GridInstance));
		m_pDeviceContext->Unmap(m_pipeline.pGridInstanceBuffer, 0);
		++m_currentFrameStats.dynamicBufferUploads;
		m_currentFrameStats.dynamicBufferUploadBytes += uploadBytes;
		m_currentFrameStats.gridUploadBytes += uploadBytes;
	}

	ID3D11Buffer* vertexBuffers[] = { m_pipeline.pQuadVertexBuffer, m_pipeline.pGridInstanceBuffer };
	UINT strides[] = { sizeof(SimpleVertex), sizeof(GridInstance) };
	UINT offsets[] = { 0, 0 };

	m_spritePipelineBound = false;
	m_pDeviceContext->IASetInputLayout(m_pipeline.pGridInputLayout);
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->VSSetShader(m_pipeline.pGridVertexShader, nullptr, 0);
	m_pDeviceContext->PSSetShader(m_pipeline.pPixelShader, nullptr, 0);
	m_pDeviceContext->PSSetSamplers(0, 1, &m_pipeline.pSampler);
	m_pDeviceContext->RSSetState(m_pipeline.pRasterizer);
	m_pDeviceContext->OMSetBlendState(m_pipeline.pBlendState, nullptr, 0xFFFFFFFF);
	m_pDeviceContext->OMSetDepthStencilState(m_pipeline.pDepthStencilState, 0);
	BindTexture(texture);
	m_pDeviceContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
	m_pDeviceContext->IASetIndexBuffer(m_pipeline.pQuadIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	SpriteTransformData transformData{ gridWorld, m_matView, m_matProjection };
	SpriteData spriteData;
	d3d::BindVertexConstantBuffer(m_pDeviceContext, m_pipeline.pTransformConstantBuffer, &transformData, sizeof(SpriteTransformData), 0);
	d3d::BindPixelConstantBuffer(m_pDeviceContext, m_pipeline.pSpriteConstantBuffer, &spriteData, sizeof(SpriteData), 0);
	m_pDeviceContext->DrawIndexedInstanced(6, static_cast<UINT>(instanceCount), 0, 0, 0);
	++m_currentFrameStats.drawCalls;
	++m_currentFrameStats.gridDrawCalls;
	m_currentFrameStats.gridInstances += static_cast<unsigned int>(instanceCount);
}

void Renderer::DrawSprite(const SpriteDesc& desc)
{
	ID3D11ShaderResourceView* texture = desc.textureFile != nullptr ? GetTexture(desc.textureFile) : m_pipeline.pWhiteTexture;
	if (texture == nullptr)
		return;

	DrawSpriteQuad(desc, texture);
}

void Renderer::DrawRectOutline(const RectOutlineDesc& desc)
{
	if (m_pipeline.pWhiteTexture == nullptr || desc.width <= 0.0f || desc.height <= 0.0f || desc.thickness <= 0.0f)
		return;

	++m_currentFrameStats.rectOutlineCalls;

	float thickness = desc.thickness;
	if (thickness > desc.width)
		thickness = desc.width;
	if (thickness > desc.height)
		thickness = desc.height;

	const float halfWidth = desc.width * 0.5f;
	const float halfHeight = desc.height * 0.5f;
	const float halfThickness = thickness * 0.5f;

	SpriteDesc lineDesc;
	lineDesc.textureFile = nullptr;
	lineDesc.depth = desc.depth;
	lineDesc.colorR = desc.colorR;
	lineDesc.colorG = desc.colorG;
	lineDesc.colorB = desc.colorB;
	lineDesc.colorA = desc.colorA;

	lineDesc.positionX = desc.positionX;
	lineDesc.positionY = desc.positionY + halfHeight - halfThickness;
	lineDesc.width = desc.width;
	lineDesc.height = thickness;
	DrawSpriteQuad(lineDesc, m_pipeline.pWhiteTexture);

	lineDesc.positionY = desc.positionY - halfHeight + halfThickness;
	DrawSpriteQuad(lineDesc, m_pipeline.pWhiteTexture);

	const float sideHeight = desc.height - thickness * 2.0f;
	if (sideHeight <= 0.0f)
		return;

	lineDesc.width = thickness;
	lineDesc.height = sideHeight;
	lineDesc.positionX = desc.positionX - halfWidth + halfThickness;
	lineDesc.positionY = desc.positionY;
	DrawSpriteQuad(lineDesc, m_pipeline.pWhiteTexture);

	lineDesc.positionX = desc.positionX + halfWidth - halfThickness;
	DrawSpriteQuad(lineDesc, m_pipeline.pWhiteTexture);
}

void Renderer::DrawGlyphSprite(const GlyphSpriteDesc& desc)
{
	if (desc.width <= 0.0f || desc.height <= 0.0f)
		return;

	GlyphBatchInstance instance;
	instance.transform = { desc.positionX, desc.positionY, desc.width, desc.height };
	instance.rows0 = {
		static_cast<float>(desc.rows[0]),
		static_cast<float>(desc.rows[1]),
		static_cast<float>(desc.rows[2]),
		static_cast<float>(desc.rows[3]) };
	instance.rows1Depth = {
		static_cast<float>(desc.rows[4]),
		static_cast<float>(desc.rows[5]),
		static_cast<float>(desc.rows[6]),
		desc.depth };
	instance.color = { desc.colorR, desc.colorG, desc.colorB, desc.colorA };
	m_glyphBatchInstances.push_back(instance);
}

void Renderer::DrawText(const TextDesc& desc)
{
	if (desc.text == nullptr || desc.text[0] == '\0' || desc.pixelSize <= 0.0f)
		return;
	if (!EnsureFontAtlas())
		return;

	++m_currentFrameStats.textDrawCalls;
	const char* scan = desc.text;
	while (*scan != '\0')
	{
		const unsigned int codepoint = DecodeUtf8Codepoint(scan);
		if (codepoint == 0)
			break;
		if (codepoint == '\r' || codepoint == '\n' || codepoint == '\t' || codepoint == ' ')
			continue;

		if (!EnsureFontGlyph(codepoint))
			EnsureFontGlyph('?');
	}
	m_currentFrameStats.fontGlyphsCached = static_cast<unsigned int>(m_fontGlyphCount);
	UploadFontAtlas();

	const float scale = desc.pixelSize * 5.0f / (std::max)(1.0f, m_fontAsciiWidth);
	const float quadWidth = static_cast<float>(m_fontAtlasCellWidth) * scale;
	const float quadHeight = static_cast<float>(m_fontAtlasCellHeight) * scale;
	const float padding = static_cast<float>(m_fontPadding) * scale;
	const float lineAdvance = desc.pixelSize * 9.0f;
	float cursorX = desc.x;
	float cursorY = desc.y;

	scan = desc.text;
	while (*scan != '\0')
	{
		const unsigned int codepoint = DecodeUtf8Codepoint(scan);
		if (codepoint == 0)
			break;

		if (codepoint == '\r')
			continue;
		if (codepoint == '\n')
		{
			cursorX = desc.x;
			cursorY -= lineAdvance;
			continue;
		}
		if (codepoint == '\t')
		{
			cursorX += desc.pixelSize * 16.0f;
			continue;
		}
		if (codepoint == ' ')
		{
			cursorX += desc.pixelSize * 4.0f;
			continue;
		}

		auto glyphIter = m_fontGlyphs.find(codepoint);
		if (glyphIter == m_fontGlyphs.end())
			glyphIter = m_fontGlyphs.find('?');
		if (glyphIter == m_fontGlyphs.end())
			continue;

		const FontGlyph& glyph = glyphIter->second;
		SpriteDesc glyphDesc;
		glyphDesc.textureFile = nullptr;
		glyphDesc.positionX = cursorX - padding + quadWidth * 0.5f;
		glyphDesc.positionY = cursorY + padding - quadHeight * 0.5f;
		glyphDesc.width = quadWidth;
		glyphDesc.height = quadHeight;
		glyphDesc.atlasColumns = m_fontAtlasColumns;
		glyphDesc.atlasRows = m_fontAtlasRows;
		glyphDesc.tileIndex = glyph.tileIndex;
		glyphDesc.colorR = desc.colorR;
		glyphDesc.colorG = desc.colorG;
		glyphDesc.colorB = desc.colorB;
		glyphDesc.colorA = desc.colorA;
		glyphDesc.depth = desc.depth;
		m_fontSpriteBatchInstances.push_back(BuildSpriteBatchInstance(glyphDesc));
		++m_currentFrameStats.textGlyphs;

		const float advanceRatio = glyph.advancePixels / (std::max)(1.0f, m_fontAsciiWidth);
		cursorX += desc.pixelSize * 6.0f * advanceRatio;
	}
}

void Renderer::DrawSpriteQuad(const SpriteDesc& desc, ID3D11ShaderResourceView* texture)
{
	if (m_pipeline.pVertexShader == nullptr || m_pipeline.pPixelShader == nullptr || texture == nullptr)
		return;

	QueueSprite(texture, desc);
}

void Renderer::GetLastFrameStats(RenderFrameStats& outStats) const
{
	outStats = m_lastFrameStats;
}

