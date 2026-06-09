#include "pch.h"
#include "Renderer.h"
#include <cmath>

using namespace DirectX;

struct SpriteTransformData
{
	XMMATRIX matWorld;
	XMMATRIX matView;
	XMMATRIX matProjection;
};

namespace
{
	constexpr size_t VerticesPerQuad = 4;
	constexpr size_t IndicesPerQuad = 6;
	constexpr size_t MaxGridQuads16Bit = 0xFFFF / VerticesPerQuad;

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

	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->IASetInputLayout(m_pipeline.pInputLayout);

}

void Renderer::BeginFrame()
{
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, m_clearColor);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);
	m_spritePipelineBound = false;
	m_boundTexture = nullptr;
}

void Renderer::EndFrame()
{
	m_pSwapChain->Present(0, 0);
}

void Renderer::InitializePipeline()
{
	SimpleVertex Rect2D[] =
	{
		{ float4(-0.5f, 0.5f, 0.0f, 1.0f), float2(0.0f, 0.0f) },
		{ float4(0.5f, 0.5f, 0.0f, 1.0f), float2(1.0f, 0.0f) },
		{ float4(0.5f,-0.5f, 0.0f, 1.0f), float2(1.0f, 1.0f) },
		{ float4(-0.5f,-0.5f, 0.0f, 1.0f), float2(0.0f, 1.0f) }
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

	SpriteTransformData transformData{ XMMatrixIdentity(), m_matView, m_matProjection };
	SpriteData spriteData;
	m_pipeline.pTransformConstantBuffer = d3d::CreateConstantBuffer(m_pDevice, &transformData, sizeof(SpriteTransformData));
	m_pipeline.pSpriteConstantBuffer = d3d::CreateConstantBuffer(m_pDevice, &spriteData, sizeof(SpriteData));
}

void Renderer::ReleasePipeline()
{
	ReleaseTextures();
	if (m_pipeline.pPixelShader)
		m_pipeline.pPixelShader->Release();
	if (m_pipeline.pVertexShader)
		m_pipeline.pVertexShader->Release();
	if (m_pipeline.pWhiteTexture)
		m_pipeline.pWhiteTexture->Release();
	if (m_pipeline.pGridIndexBuffer)
		m_pipeline.pGridIndexBuffer->Release();
	if (m_pipeline.pGridVertexBuffer)
		m_pipeline.pGridVertexBuffer->Release();
	if (m_pipeline.pSpriteConstantBuffer)
		m_pipeline.pSpriteConstantBuffer->Release();
	if (m_pipeline.pTransformConstantBuffer)
		m_pipeline.pTransformConstantBuffer->Release();
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
}

void Renderer::EnsureGridBatchCapacity(size_t quadCount)
{
	if (quadCount == 0 || quadCount <= m_gridQuadCapacity)
		return;

	if (m_pipeline.pGridVertexBuffer)
	{
		m_pipeline.pGridVertexBuffer->Release();
		m_pipeline.pGridVertexBuffer = nullptr;
	}

	if (m_pipeline.pGridIndexBuffer)
	{
		m_pipeline.pGridIndexBuffer->Release();
		m_pipeline.pGridIndexBuffer = nullptr;
	}

	const size_t doubledCapacity = m_gridQuadCapacity > 0 ? m_gridQuadCapacity * 2 : 256;
	m_gridQuadCapacity = quadCount > doubledCapacity ? quadCount : doubledCapacity;
	if (m_gridQuadCapacity > MaxGridQuads16Bit)
		m_gridQuadCapacity = MaxGridQuads16Bit;

	m_pipeline.pGridVertexBuffer = CreateDynamicBuffer(m_pDevice,
		static_cast<UINT>(m_gridQuadCapacity * VerticesPerQuad * sizeof(SimpleVertex)),
		D3D11_BIND_VERTEX_BUFFER);

	m_pipeline.pGridIndexBuffer = CreateDynamicBuffer(m_pDevice,
		static_cast<UINT>(m_gridQuadCapacity * IndicesPerQuad * sizeof(USHORT)),
		D3D11_BIND_INDEX_BUFFER);
}

void Renderer::LoadTexture(const WCHAR* textureFile)
{
	m_pipeline.pTexture = GetTexture(textureFile);
}

ID3D11ShaderResourceView* Renderer::GetTexture(const WCHAR* textureFile)
{
	if (textureFile == nullptr)
		return nullptr;

	std::wstring key(textureFile);
	auto iter = m_textureMap.find(key);
	if (iter != m_textureMap.end())
		return iter->second;

	WCHAR wszFullPath[MAX_PATH] = {};
	GetCurrentDirectoryW(MAX_PATH, wszFullPath);
	PathAppendW(wszFullPath, textureFile);

	if (!PathFileExistsW(wszFullPath))
	{
		WCHAR wszModuleDir[MAX_PATH] = {};
		GetModuleFileNameW(nullptr, wszModuleDir, MAX_PATH);
		PathRemoveFileSpecW(wszModuleDir);
		PathAppendW(wszModuleDir, L"..\\..\\..");
		PathCanonicalizeW(wszFullPath, wszModuleDir);
		PathAppendW(wszFullPath, textureFile);
	}

	if (!PathFileExistsW(wszFullPath))
	{
		OutputDebugStringW(L"Texture file was not found: ");
		OutputDebugStringW(wszFullPath);
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
	return pSRV;
}

void Renderer::LoadPipelineShader(const WCHAR* shaderFile)
{
	WCHAR* wszCurrentDir = new WCHAR[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, wszCurrentDir);
	PathAppendW(wszCurrentDir, shaderFile);
	if (!PathFileExistsW(wszCurrentDir))
	{
		WCHAR wszModuleDir[MAX_PATH] = {};
		GetModuleFileNameW(nullptr, wszModuleDir, MAX_PATH);
		PathRemoveFileSpecW(wszModuleDir);
		PathAppendW(wszModuleDir, L"..\\..\\..");
		PathCanonicalizeW(wszCurrentDir, wszModuleDir);
		PathAppendW(wszCurrentDir, shaderFile);
	}
	if (!PathFileExistsW(wszCurrentDir))
	{
		OutputDebugStringW(L"Shader file was not found: ");
		OutputDebugStringW(wszCurrentDir);
		OutputDebugStringW(L"\n");
		__debugbreak();
	}

	WCHAR* wszFileName = PathFindFileNameW(shaderFile);
	char* szFileName = new char[256];
	WideCharToMultiByte(CP_UTF8, 0, wszFileName, -1, szFileName, 256, NULL, NULL);

	char* szMainName = new char[256];
	int len = 0;
	char* pExtention = szFileName;
	while (*pExtention != '.')
	{
		len++;
		pExtention++;
	}
	memcpy_s(szMainName, len, szFileName, len);
	szMainName[len] = '_';
	szMainName[len + 1] = 'V';
	szMainName[len + 2] = 'S';
	szMainName[len + 3] = '\0';

	ID3DBlob* pVertexShaderBlob = nullptr;
	ID3DBlob* pPixelShaderBlob = nullptr;
	ID3DBlob* pErrorBlob = nullptr;

	int flag = 0;
#ifdef _DEBUG
	flag = D3DCOMPILE_DEBUG;
#endif
	flag |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

	HRESULT hr = D3DCompileFromFile(wszCurrentDir, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, szMainName, "vs_5_0", flag, 0, &pVertexShaderBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			char* error = static_cast<char*>(pErrorBlob->GetBufferPointer());
			OutputDebugStringA(error);
			pErrorBlob->Release();
			pErrorBlob = nullptr;
		}
		else
		{
			OutputDebugStringW(L"Vertex shader compile failed, but no error blob was returned: ");
			OutputDebugStringW(wszCurrentDir);
			OutputDebugStringW(L"\n");
		}
		__debugbreak();
	}
	if (S_OK != m_pDevice->CreateVertexShader(pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize(), nullptr, &m_pipeline.pVertexShader))
		__debugbreak();

	szMainName[len + 1] = 'P';
	hr = D3DCompileFromFile(wszCurrentDir, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, szMainName, "ps_5_0", flag, 0, &pPixelShaderBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			char* error = static_cast<char*>(pErrorBlob->GetBufferPointer());
			OutputDebugStringA(error);
			pErrorBlob->Release();
			pErrorBlob = nullptr;
		}
		else
		{
			OutputDebugStringW(L"Pixel shader compile failed, but no error blob was returned: ");
			OutputDebugStringW(wszCurrentDir);
			OutputDebugStringW(L"\n");
		}
		__debugbreak();
	}
	if (S_OK != m_pDevice->CreatePixelShader(pPixelShaderBlob->GetBufferPointer(), pPixelShaderBlob->GetBufferSize(), nullptr, &m_pipeline.pPixelShader))
		__debugbreak();

	m_pVertexShaderBlob = pVertexShaderBlob;

	pPixelShaderBlob->Release();
	delete[] szMainName;
	delete[] szFileName;
	delete[] wszCurrentDir;
}

void Renderer::DrawBlockGrid(const BlockGridDesc& desc)
{
	if (desc.textureFile == nullptr || desc.tiles == nullptr || desc.width <= 0 || desc.height <= 0)
		return;

	ID3D11ShaderResourceView* texture = GetTexture(desc.textureFile);
	if (m_pipeline.pVertexShader == nullptr || m_pipeline.pPixelShader == nullptr || texture == nullptr)
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

	m_gridVertices.clear();
	m_gridIndices.clear();
	m_gridVertices.reserve(desc.width * desc.height * VerticesPerQuad);
	m_gridIndices.reserve(desc.width * desc.height * IndicesPerQuad);

	for (int y = 0; y < desc.height; ++y)
	{
		for (int x = 0; x < desc.width; ++x)
		{
			const BlockTile& tile = desc.tiles[y * desc.width + x];
			if (!tile.visible)
				continue;

			const float centerX = desc.originX + x * desc.tileSize;
			const float centerY = desc.originY - y * desc.tileSize;
			if (centerX < minVisibleX || centerX > maxVisibleX || centerY < minVisibleY || centerY > maxVisibleY)
				continue;

			if (m_gridVertices.size() + VerticesPerQuad > 0xFFFF)
				break;

			int tileIndex = tile.tileIndex;
			if (tileCount > 0)
				tileIndex = tileIndex % tileCount;

			const int tileX = tileIndex % atlasColumns;
			const int tileY = tileIndex / atlasColumns;
			const float u0 = tileX * uvWidth;
			const float v0 = tileY * uvHeight;
			const float u1 = u0 + uvWidth;
			const float v1 = v0 + uvHeight;
			const float left = centerX - halfTile;
			const float right = centerX + halfTile;
			const float top = centerY + halfTile;
			const float bottom = centerY - halfTile;
			const USHORT baseVertex = static_cast<USHORT>(m_gridVertices.size());

			m_gridVertices.push_back({ float4(left, top, 0.0f, 1.0f), float2(u0, v0) });
			m_gridVertices.push_back({ float4(right, top, 0.0f, 1.0f), float2(u1, v0) });
			m_gridVertices.push_back({ float4(right, bottom, 0.0f, 1.0f), float2(u1, v1) });
			m_gridVertices.push_back({ float4(left, bottom, 0.0f, 1.0f), float2(u0, v1) });

			m_gridIndices.push_back(baseVertex);
			m_gridIndices.push_back(baseVertex + 1);
			m_gridIndices.push_back(baseVertex + 2);
			m_gridIndices.push_back(baseVertex);
			m_gridIndices.push_back(baseVertex + 2);
			m_gridIndices.push_back(baseVertex + 3);
		}
	}

	if (m_gridIndices.empty())
		return;

	const size_t quadCount = m_gridVertices.size() / VerticesPerQuad;
	EnsureGridBatchCapacity(quadCount);

	D3D11_MAPPED_SUBRESOURCE vertexResource;
	if (S_OK != m_pDeviceContext->Map(m_pipeline.pGridVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &vertexResource))
		__debugbreak();
	memcpy_s(vertexResource.pData, m_gridQuadCapacity * VerticesPerQuad * sizeof(SimpleVertex),
		m_gridVertices.data(), m_gridVertices.size() * sizeof(SimpleVertex));
	m_pDeviceContext->Unmap(m_pipeline.pGridVertexBuffer, 0);

	D3D11_MAPPED_SUBRESOURCE indexResource;
	if (S_OK != m_pDeviceContext->Map(m_pipeline.pGridIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &indexResource))
		__debugbreak();
	memcpy_s(indexResource.pData, m_gridQuadCapacity * IndicesPerQuad * sizeof(USHORT),
		m_gridIndices.data(), m_gridIndices.size() * sizeof(USHORT));
	m_pDeviceContext->Unmap(m_pipeline.pGridIndexBuffer, 0);

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	BindSpritePipeline();
	BindTexture(texture);
	m_pDeviceContext->IASetVertexBuffers(0, 1, &m_pipeline.pGridVertexBuffer, &stride, &offset);
	m_pDeviceContext->IASetIndexBuffer(m_pipeline.pGridIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	SpriteTransformData transformData{ XMMatrixIdentity(), m_matView, m_matProjection };
	SpriteData spriteData;
	d3d::BindVertexConstantBuffer(m_pDeviceContext, m_pipeline.pTransformConstantBuffer, &transformData, sizeof(SpriteTransformData), 0);
	d3d::BindPixelConstantBuffer(m_pDeviceContext, m_pipeline.pSpriteConstantBuffer, &spriteData, sizeof(SpriteData), 0);
	m_pDeviceContext->DrawIndexed(static_cast<UINT>(m_gridIndices.size()), 0, 0);
}

void Renderer::DrawSprite(const SpriteDesc& desc)
{
	if (desc.textureFile == nullptr)
		return;

	ID3D11ShaderResourceView* texture = GetTexture(desc.textureFile);
	if (texture == nullptr)
		return;

	DrawSpriteQuad(desc, texture);
}

void Renderer::DrawRectOutline(const RectOutlineDesc& desc)
{
	if (m_pipeline.pWhiteTexture == nullptr || desc.width <= 0.0f || desc.height <= 0.0f || desc.thickness <= 0.0f)
		return;

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

void Renderer::DrawSpriteQuad(const SpriteDesc& desc, ID3D11ShaderResourceView* texture)
{
	if (m_pipeline.pVertexShader == nullptr || m_pipeline.pPixelShader == nullptr || texture == nullptr)
		return;

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	BindSpritePipeline();
	m_pDeviceContext->IASetVertexBuffers(0, 1, &m_pipeline.pQuadVertexBuffer, &stride, &offset);
	m_pDeviceContext->IASetIndexBuffer(m_pipeline.pQuadIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	BindTexture(texture);

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

	SpriteData spriteData;
	spriteData.ratio = { uvWidth, uvHeight };
	spriteData.offset = { tileX * uvWidth, tileY * uvHeight };
	if (desc.flipX)
	{
		spriteData.ratio.x = -uvWidth;
		spriteData.offset.x = (tileX + 1) * uvWidth;
	}

	XMMATRIX world = XMMatrixScaling(desc.width, desc.height, 1.0f) * XMMatrixTranslation(desc.positionX, desc.positionY, desc.depth);

	SpriteTransformData transformData{ world, m_matView, m_matProjection };
	d3d::BindVertexConstantBuffer(m_pDeviceContext, m_pipeline.pTransformConstantBuffer, &transformData, sizeof(SpriteTransformData), 0);
	d3d::BindPixelConstantBuffer(m_pDeviceContext, m_pipeline.pSpriteConstantBuffer, &spriteData, sizeof(SpriteData), 0);
	m_pDeviceContext->DrawIndexed(6, 0, 0);
}

