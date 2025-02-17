#include "pch.h"
#include "GameObject.h"

unsigned int GameObject::globalID = 0;

GameObject::GameObject()
{
	ID = globalID++;
}


void GameObject::Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12RootSignature* pRootSignature)
{
	//mMesh.Initialize(pDevice, pCommandList);
	//mShader.Initialize(pDevice, pRootSignature);
}

void GameObject::Render(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* pCbvHeap)
{
	//mShader.SetPipelineState(pCommandList);
	//mMesh.Render(pCommandList, pCbvHeap, pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
}
