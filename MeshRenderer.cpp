#include "pch.h"
#include "MeshRenderer.h"

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
	mMesh->SetBuffers(pCommandList);

	int offset = 0;
	for (int i = 0; i < mSubMeshCount; ++i)
	{
		mMaterials[i]->SetConstantBufferView(pCommandList, pSrvHeap);
		mMesh->RenderSubMeshes(pCommandList, i);
	}
}
