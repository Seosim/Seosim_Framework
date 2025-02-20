#pragma once
#include "pch.h"
#include "Material.h"
#include "Mesh.h"
#include "Transform.h"

class Skybox {
public:
	Skybox() = default;
	~Skybox() { delete mpMaterial; delete mpMesh; }
	Skybox(const Skybox&) = delete;

	void Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12RootSignature* pRootSignature, ID3D12DescriptorHeap* pSrvHeap);

	void Render(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* pSrvHeap);

	CTransform mTransform;
private:
	Material* mpMaterial = nullptr;
	Mesh* mpMesh = nullptr;
};
