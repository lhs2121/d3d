#include "pch.h"
#include "d3d.h"

namespace
{
	void UpdateConstantBuffer(ID3D11DeviceContext* pDeviceContext, ID3D11Buffer* pBuffer, const void* pData, UINT size)
	{
		D3D11_MAPPED_SUBRESOURCE subResource;

		if (S_OK != pDeviceContext->Map(pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource))
			__debugbreak();

		memcpy_s(subResource.pData, size, pData, size);
		pDeviceContext->Unmap(pBuffer, 0);
	}
}

ID3D11Buffer* d3d::CreateVertexBuffer(ID3D11Device* pDevice, void* pList, UINT size, UINT stride)
{
	ID3D11Buffer* pResult;
	D3D11_BUFFER_DESC vbDesc = { size, D3D11_USAGE_DEFAULT,D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
	D3D11_SUBRESOURCE_DATA vbData = { pList, 0, 0 };

	if (S_OK != pDevice->CreateBuffer(&vbDesc, &vbData, &pResult))
		__debugbreak();

	return pResult;
}

ID3D11Buffer* d3d::CreateIndexBuffer(ID3D11Device* pDevice, void* pList, UINT size, UINT stride)
{
	ID3D11Buffer* pResult;
	D3D11_BUFFER_DESC ibDesc = { size, D3D11_USAGE_DEFAULT,D3D10_BIND_INDEX_BUFFER, 0, 0, 0 };
	D3D11_SUBRESOURCE_DATA ibData = { pList, 0, 0 };

	if (S_OK != pDevice->CreateBuffer(&ibDesc, &ibData, &pResult))
		__debugbreak();

	return pResult;
}

ID3D11Buffer* d3d::CreateConstantBuffer(ID3D11Device* pDevice, const void* pData, UINT size)
{
	ID3D11Buffer* pBuffer = nullptr;

	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = size;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA subData;
	subData.pSysMem = pData;
	subData.SysMemPitch = 0;
	subData.SysMemSlicePitch = 0;

	if (S_OK != pDevice->CreateBuffer(&desc, &subData, &pBuffer))
		__debugbreak();

	return pBuffer;
}

ID3D11RasterizerState* d3d::CreateRasterizerState(ID3D11Device* pDevice)
{
	ID3D11RasterizerState* pResult = nullptr;

	D3D11_RASTERIZER_DESC desc = {};
	desc.FillMode = D3D11_FILL_SOLID;
	desc.CullMode = D3D11_CULL_BACK;
	desc.FrontCounterClockwise = false;
	desc.DepthBias = 0;
	desc.DepthBiasClamp = 0;
	desc.SlopeScaledDepthBias = 0;
	desc.DepthClipEnable = true;
	desc.ScissorEnable = false;
	desc.MultisampleEnable = true;
	desc.AntialiasedLineEnable = false;

	if (S_OK != pDevice->CreateRasterizerState(&desc, &pResult))
		__debugbreak();

	return pResult;
}

ID3D11SamplerState* d3d::CreateSamplerState(ID3D11Device* pDevice)
{
	ID3D11SamplerState* pResult = nullptr;

	D3D11_SAMPLER_DESC desc = {};
	desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.MipLODBias = 0.0f;
	desc.MaxAnisotropy = 1;
	desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	desc.BorderColor[0] = 1.0f;
	desc.BorderColor[1] = 1.0f;
	desc.BorderColor[2] = 1.0f;
	desc.BorderColor[3] = 1.0f;
	desc.MinLOD = 0;
	desc.MaxLOD = D3D11_FLOAT32_MAX;

	if (S_OK != pDevice->CreateSamplerState(&desc, &pResult))
		__debugbreak();

	return pResult;
}

ID3D11BlendState* d3d::CreateBlendState(ID3D11Device* pDevice)
{
	ID3D11BlendState* pResult = nullptr;

	D3D11_BLEND_DESC desc = { 0 };
	desc.AlphaToCoverageEnable = true;
	desc.IndependentBlendEnable = false;
	desc.RenderTarget[0].BlendEnable = true;
	desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	if (S_OK != pDevice->CreateBlendState(&desc, &pResult))
		__debugbreak();

	return pResult;
}

ID3D11DepthStencilState* d3d::CreateDepthStencilState(ID3D11Device* pDevice)
{
	ID3D11DepthStencilState* pResult = nullptr;

	D3D11_DEPTH_STENCIL_DESC desc = { 0 };
	desc.DepthEnable = true;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	desc.StencilEnable = false;

	if (S_OK != pDevice->CreateDepthStencilState(&desc, &pResult))
		__debugbreak();

	return pResult;
}

void d3d::BindVertexConstantBuffer(ID3D11DeviceContext* pDeviceContext, ID3D11Buffer* pBuffer, const void* pData, UINT size, UINT slot)
{
	UpdateConstantBuffer(pDeviceContext, pBuffer, pData, size);
	pDeviceContext->VSSetConstantBuffers(slot, 1, &pBuffer);
}

void d3d::BindPixelConstantBuffer(ID3D11DeviceContext* pDeviceContext, ID3D11Buffer* pBuffer, const void* pData, UINT size, UINT slot)
{
	UpdateConstantBuffer(pDeviceContext, pBuffer, pData, size);
	pDeviceContext->PSSetConstantBuffers(slot, 1, &pBuffer);
}
