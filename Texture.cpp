#include "pch.h"
#include "Texture.h"

void Texture::LoadTextureFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszFileName, UINT nResourceType, UINT nIndex)
{
	mResourceType = nResourceType;
	mpResource = CreateTextureResourceFromDDSFile(pd3dDevice, pd3dCommandList, pszFileName, &mpUploadBuffer, D3D12_RESOURCE_STATE_GENERIC_READ/*D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE*/);
}

void Texture::CreateSrv(ID3D12Device* pDevice, ID3D12DescriptorHeap* pSrvHeap)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(pSrvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mpResource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = mpResource->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	pDevice->CreateShaderResourceView(mpResource, &srvDesc, hDescriptor);
}
