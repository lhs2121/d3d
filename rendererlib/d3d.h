#pragma once
#include "typedef.h"

namespace d3d
{
	ID3D11Buffer* CreateVertexBuffer(ID3D11Device* pDevice, void* pList, UINT size, UINT stride);
	ID3D11Buffer* CreateIndexBuffer(ID3D11Device* pDevice, void* pList, UINT size, UINT stride);
	ID3D11Buffer* CreateConstantBuffer(ID3D11Device* pDevice, const void* pData, UINT size);
	ID3D11RasterizerState* CreateRasterizerState(ID3D11Device* pDevice);
	ID3D11SamplerState* CreateSamplerState(ID3D11Device* pDevice);
	ID3D11BlendState* CreateBlendState(ID3D11Device* pDevice);
	ID3D11DepthStencilState* CreateDepthStencilState(ID3D11Device* pDevice);
	void BindVertexConstantBuffer(ID3D11DeviceContext* pDeviceContext, ID3D11Buffer* pBuffer, const void* pData, UINT size, UINT slot);
	void BindPixelConstantBuffer(ID3D11DeviceContext* pDeviceContext, ID3D11Buffer* pBuffer, const void* pData, UINT size, UINT slot);
}
