#include "pch.h"
#include "MeshRenderer.h"

Material* MeshRenderer::PrevUsedMaterial = nullptr;
Mesh* MeshRenderer::PrevUsedMesh = nullptr;

void MeshRenderer::Update(const float deltaTime)
{
}

void MeshRenderer::SetMesh(Mesh* mesh)
{
	ASSERT(mMesh == nullptr);	//메쉬가 이미 등록되어 있을 시 사용 x

	mMesh = mesh;
	mSubMeshCount = mMesh->GetSubMeshCount();
}

void MeshRenderer::AddMaterial(Material* material)
{
	mMaterials.push_back(material);
}

Material* MeshRenderer::GetMaterial(int index) const
{
	return mMaterials[index];
}

Mesh* MeshRenderer::GetMesh() const
{
	return mMesh;
}

void MeshRenderer::Render(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* pSrvHeap)
{
	if (mMesh != MeshRenderer::PrevUsedMesh)
	{
		MeshRenderer::PrevUsedMesh = mMesh;
		mMesh->SetBuffers(pCommandList);
	}

	for (int i = 0; i < mSubMeshCount; ++i)
	{
		if (mMaterials[i] != MeshRenderer::PrevUsedMaterial)
		{
			mMaterials[i]->SetConstantBufferView(pCommandList, pSrvHeap);
			MeshRenderer::PrevUsedMaterial = mMaterials[i];
		}
		else
		{
			int a = 10;
		}
		mMesh->RenderSubMeshes(pCommandList, i);
	}
}
