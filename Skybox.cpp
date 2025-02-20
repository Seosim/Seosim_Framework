#include "pch.h"
#include "Skybox.h"

void Skybox::Initialize(ID3D12Device* pDevice,ID3D12GraphicsCommandList* pCommandList, ID3D12RootSignature* pRootSignature, ID3D12DescriptorHeap* pSrvHeap)
{
	mpMaterial = new Material();
	mpMaterial->Initialize(pDevice, pCommandList, pRootSignature, pSrvHeap, NULL, Shader::eType::Skybox);

	mpMesh = new Mesh();
	mpMesh->LoadMeshData(pDevice, pCommandList, "./Assets/Models/cube.bin");
}

void Skybox::Render(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* pSrvHeap)
{
	mpMaterial->SetConstantBufferView(pCommandList, pSrvHeap);
	mpMesh->Render(pCommandList);
}
