#include "pch.h"
#include "Texture.h"

std::unordered_map<std::wstring, Texture*> Texture::TextureList;
UINT Texture::mCbvSrvUavDescriptorSize;

void Texture::LoadTextureFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszFileName, UINT nResourceType, UINT nIndex)
{
	mResourceType = nResourceType;
	mpResource = CreateTextureResourceFromDDSFile(pd3dDevice, pd3dCommandList, pszFileName, &mpUploadBuffer, D3D12_RESOURCE_STATE_GENERIC_READ/*D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE*/);
}

void Texture::CreateSrv(ID3D12Device* pDevice, ID3D12DescriptorHeap* pSrvHeap, const std::wstring& name, bool isCubeMap)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(pSrvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mpResource->GetDesc().Format;

	if (isCubeMap)
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = mpResource->GetDesc().MipLevels;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	}
	else
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = mpResource->GetDesc().MipLevels;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	}


	const int textureIndex = Texture::TextureList.size();
	ID = textureIndex;

	hDescriptor.ptr += mCbvSrvUavDescriptorSize * ID;

	pDevice->CreateShaderResourceView(mpResource, &srvDesc, hDescriptor);

	Texture::TextureList[name] = this;
}

void Texture::CreateSrvWithResource(ID3D12Device* pDevice, ID3D12DescriptorHeap* pSrvHeap, const std::wstring& name, ID3D12Resource* pResource, const DXGI_FORMAT format, bool bMSAA)
{
	mpResource = pResource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = format;
	srvDesc.ViewDimension = bMSAA ? D3D12_SRV_DIMENSION_TEXTURE2DMS : D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = mpResource->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	const int textureIndex = Texture::TextureList.size();
	ID = textureIndex;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(pSrvHeap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.ptr += mCbvSrvUavDescriptorSize * ID;

	pDevice->CreateShaderResourceView(mpResource, &srvDesc, hDescriptor);

	Texture::TextureList[name] = this;
}

void Texture::InitializeUAV(ID3D12Device* pDevice, ID3D12DescriptorHeap* pSrvHeap, const DXGI_FORMAT format, const std::wstring& name, const UINT width, const UINT height)
{
	if (ID == -1)
	{
		const int textureIndex = Texture::TextureList.size();
		ID = textureIndex;

		Texture::TextureList[name] = this;

		//SRV와 UAV로 사용하기 위해 텍스처리스트에 2가지 KEY를 넣습니다.
		std::wstring uavName = name + L"UAV";
		Texture::TextureList[uavName] = nullptr;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(pSrvHeap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.ptr += mCbvSrvUavDescriptorSize * ID;

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	auto heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	HR(pDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&mpResource)));

	//PostProcessing (SRV)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;

		pDevice->CreateShaderResourceView(mpResource, &srvDesc, hDescriptor);
		hDescriptor.ptr += mCbvSrvUavDescriptorSize;
	}

	//PostProcessing (UAV)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

		uavDesc.Format = format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;

		pDevice->CreateUnorderedAccessView(mpResource, nullptr, &uavDesc, hDescriptor);
	}
}

void Texture::ChangeResource(ID3D12Device* pDevice, ID3D12DescriptorHeap* pSrvHeap, const std::wstring& name, ID3D12Resource* pResource, const DXGI_FORMAT format, bool bMSAA)
{
	mpResource = pResource;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(pSrvHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = format;
	srvDesc.ViewDimension = bMSAA ? D3D12_SRV_DIMENSION_TEXTURE2DMS : D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = mpResource->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	hDescriptor.ptr += mCbvSrvUavDescriptorSize * ID;
	pDevice->CreateShaderResourceView(mpResource, &srvDesc, hDescriptor);

	Texture::TextureList[name] = this;
}

ID3D12Resource* Texture::GetResource()
{
	return mpResource;
}

int Texture::GetID() const
{
	return ID;
}