#pragma once
#include "pch.h"
#include "UploadBuffer.h"
#include "DDSTextureLoader12.h"

#define RESOURCE_TEXTURE2D			0x01
#define RESOURCE_TEXTURE2D_ARRAY	0x02	//[]
#define RESOURCE_TEXTURE2DARRAY		0x03
#define RESOURCE_TEXTURE_CUBE		0x04
#define RESOURCE_BUFFER				0x05

class Texture {
public:
	Texture() = default;
    ~Texture() { RELEASE_COM(mpResource); RELEASE_COM(mpUploadBuffer); }
	Texture(const Texture&) = delete;

	void LoadTextureFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszFileName, UINT nResourceType, UINT nIndex);
    void CreateSrv(ID3D12Device* pDevice, ID3D12DescriptorHeap* pSrvHeap, const std::wstring& name, bool isCubeMap = false);
    void CreateSrvWithResource(ID3D12Device* pDevice, ID3D12DescriptorHeap* pSrvHeap, const std::wstring& name, ID3D12Resource* pResource, const DXGI_FORMAT format, bool bMSAA = true);

    void ChangeResource(ID3D12Device* pDevice, ID3D12DescriptorHeap* pSrvHeap, const std::wstring& name, ID3D12Resource* pResource, const DXGI_FORMAT format, bool bMSAA = true);

    ID3D12Resource* GetResource();

    int GetID() const;

    static std::unordered_map<std::wstring, Texture*> TextureList;
	ID3D12Resource* mpResource = nullptr;
private:
	ID3D12Resource* mpUploadBuffer = nullptr;
	UINT mResourceType = {};
    int ID = -1;
};

static ID3D12Resource* CreateTextureResourceFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszFileName, ID3D12Resource** ppd3dUploadBuffer, D3D12_RESOURCE_STATES d3dResourceStates)
{
    ID3D12Resource* pd3dTexture = NULL;
    std::unique_ptr<uint8_t[]> ddsData;
    std::vector<D3D12_SUBRESOURCE_DATA> vSubresources;
    DDS_ALPHA_MODE ddsAlphaMode = DDS_ALPHA_MODE_UNKNOWN;
    bool bIsCubeMap = false;

    HRESULT hResult = DirectX::LoadDDSTextureFromFileEx(pd3dDevice, pszFileName, 0, D3D12_RESOURCE_FLAG_NONE, DDS_LOADER_DEFAULT, &pd3dTexture, ddsData, vSubresources, &ddsAlphaMode, &bIsCubeMap);

    D3D12_HEAP_PROPERTIES d3dHeapPropertiesDesc;
    ::ZeroMemory(&d3dHeapPropertiesDesc, sizeof(D3D12_HEAP_PROPERTIES));
    d3dHeapPropertiesDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
    d3dHeapPropertiesDesc.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    d3dHeapPropertiesDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    d3dHeapPropertiesDesc.CreationNodeMask = 1;
    d3dHeapPropertiesDesc.VisibleNodeMask = 1;

    //	D3D12_RESOURCE_DESC d3dResourceDesc = pd3dTexture->GetDesc();
    //	UINT nSubResources = d3dResourceDesc.DepthOrArraySize * d3dResourceDesc.MipLevels;
    UINT nSubResources = (UINT)vSubresources.size();
    //	UINT64 nBytes = 0;
    //	pd3dDevice->GetCopyableFootprints(&d3dResourceDesc, 0, nSubResources, 0, NULL, NULL, NULL, &nBytes);
    UINT64 nBytes = GetRequiredIntermediateSize(pd3dTexture, 0, nSubResources);

    D3D12_RESOURCE_DESC d3dResourceDesc;
    ::ZeroMemory(&d3dResourceDesc, sizeof(D3D12_RESOURCE_DESC));
    d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; //Upload Heap에는 텍스쳐를 생성할 수 없음
    d3dResourceDesc.Alignment = 0;
    d3dResourceDesc.Width = nBytes;
    d3dResourceDesc.Height = 1;
    d3dResourceDesc.DepthOrArraySize = 1;
    d3dResourceDesc.MipLevels = 1;
    d3dResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    d3dResourceDesc.SampleDesc.Count = 1;
    d3dResourceDesc.SampleDesc.Quality = 0;
    d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    pd3dDevice->CreateCommittedResource(&d3dHeapPropertiesDesc, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, __uuidof(ID3D12Resource), (void**)ppd3dUploadBuffer);

    //UINT nSubResources = (UINT)vSubresources.size();
    //D3D12_SUBRESOURCE_DATA *pd3dSubResourceData = new D3D12_SUBRESOURCE_DATA[nSubResources];
    //for (UINT i = 0; i < nSubResources; i++) pd3dSubResourceData[i] = vSubresources.at(i);

    //	std::vector<D3D12_SUBRESOURCE_DATA>::pointer ptr = &vSubresources[0];
    UpdateSubresources(pd3dCommandList, pd3dTexture, *ppd3dUploadBuffer, 0, 0, nSubResources, &vSubresources[0]);

    D3D12_RESOURCE_BARRIER d3dResourceBarrier;
    ::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
    d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    d3dResourceBarrier.Transition.pResource = pd3dTexture;
    d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    d3dResourceBarrier.Transition.StateAfter = d3dResourceStates;
    d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

    //	delete[] pd3dSubResourceData;

    return(pd3dTexture);
}