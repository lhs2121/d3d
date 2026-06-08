#include "pch.h"
#include "Renderer.h"

using namespace DirectX;

struct SpriteTransformData
{
	XMMATRIX matWorld;
	XMMATRIX matView;
	XMMATRIX matProjection;
};

Renderer::~Renderer()
{
	ReleasePipeline();

	if (m_pDeviceContext)
		m_pDeviceContext->ClearState();

	m_pDeviceContext->Release();
	m_pSwapChain->Release();
	m_pRenderTargetBuffer->Release();
	m_pDepthStencilBuffer->Release();
	m_pDepthStencilView->Release();
	m_pRenderTargetView->Release();

#if defined(DEBUG) || defined(_DEBUG)
	ID3D11Debug* dxgiDebug;

	if (S_OK == m_pDevice->QueryInterface(IID_PPV_ARGS(&dxgiDebug)))
	{
		dxgiDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
		dxgiDebug->Release();
	}
#endif

	m_pDevice->Release();
}

void Renderer::Initialize(UINT winWidth, UINT winHeight, HWND& hwnd)
{
	IDXGIFactory* pFact = nullptr;
	IDXGIAdapter* pAdap = nullptr;

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

	CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFact));
	pFact->EnumAdapters(0, &pAdap);

	HRESULT rs = D3D11CreateDevice(pAdap, D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_UNKNOWN, nullptr,
		D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, &m_pDevice, &featureLevel, &m_pDeviceContext);
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

	InitializePipeline();

	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->IASetInputLayout(m_pipeline.pInputLayout);

}

void Renderer::BeginFrame()
{
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, m_clearColor);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);
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

	LoadPipelineShader(L"Assets\\Shaders\\BasicSprite2DShader.hlsl");

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
}

void Renderer::ReleasePipeline()
{
	ReleaseTextures();
	if (m_pipeline.pPixelShader)
		m_pipeline.pPixelShader->Release();
	if (m_pipeline.pVertexShader)
		m_pipeline.pVertexShader->Release();
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

	for (int y = 0; y < desc.height; ++y)
	{
		for (int x = 0; x < desc.width; ++x)
		{
			const BlockTile& tile = desc.tiles[y * desc.width + x];
			if (!tile.visible)
				continue;

			SpriteDesc spriteDesc;
			spriteDesc.positionX = desc.originX + x * desc.tileSize;
			spriteDesc.positionY = desc.originY - y * desc.tileSize;
			spriteDesc.width = desc.tileSize;
			spriteDesc.height = desc.tileSize;
			spriteDesc.atlasColumns = desc.atlasColumns;
			spriteDesc.atlasRows = desc.atlasRows;
			spriteDesc.tileIndex = tile.tileIndex;
			spriteDesc.depth = 0.0f;

			DrawSpriteQuad(spriteDesc, texture);
		}
	}
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

void Renderer::DrawSpriteQuad(const SpriteDesc& desc, ID3D11ShaderResourceView* texture)
{
	if (m_pipeline.pVertexShader == nullptr || m_pipeline.pPixelShader == nullptr || texture == nullptr)
		return;

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	m_pDeviceContext->IASetInputLayout(m_pipeline.pInputLayout);
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pDeviceContext->IASetVertexBuffers(0, 1, &m_pipeline.pQuadVertexBuffer, &stride, &offset);
	m_pDeviceContext->IASetIndexBuffer(m_pipeline.pQuadIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	m_pDeviceContext->VSSetShader(m_pipeline.pVertexShader, nullptr, 0);
	m_pDeviceContext->PSSetShader(m_pipeline.pPixelShader, nullptr, 0);
	m_pDeviceContext->PSSetShaderResources(0, 1, &texture);
	m_pDeviceContext->PSSetSamplers(0, 1, &m_pipeline.pSampler);
	m_pDeviceContext->RSSetState(m_pipeline.pRasterizer);
	m_pDeviceContext->OMSetBlendState(m_pipeline.pBlendState, nullptr, 0xFFFFFFFF);
	m_pDeviceContext->OMSetDepthStencilState(m_pipeline.pDepthStencilState, 0);

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
	ID3D11Buffer* transformBuffer = d3d::CreateConstantBuffer(m_pDevice, &transformData, sizeof(SpriteTransformData));
	ID3D11Buffer* spriteBuffer = d3d::CreateConstantBuffer(m_pDevice, &spriteData, sizeof(SpriteData));

	d3d::BindVertexConstantBuffer(m_pDeviceContext, transformBuffer, &transformData, sizeof(SpriteTransformData), 0);
	d3d::BindPixelConstantBuffer(m_pDeviceContext, spriteBuffer, &spriteData, sizeof(SpriteData), 0);
	m_pDeviceContext->DrawIndexed(6, 0, 0);

	transformBuffer->Release();
	spriteBuffer->Release();
}

