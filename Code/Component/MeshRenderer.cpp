#include "pch.h"
#include "MeshRenderer.h"

void MeshRenderer::Update(const float deltaTime)
{
}

void MeshRenderer::SetOtherComponent(Mesh* mesh, Transform* transform)
{
	SetTransform(transform);
	SetMesh(mesh);

	auto worldMatrix = mTransform->GetWorldTransform();
	mTranslatedCullingBox = mMesh->mFrustumCullingBox;
	mTranslatedCullingBox.Transform(mTranslatedCullingBox, worldMatrix);
}

void MeshRenderer::SetMesh(Mesh* mesh)
{
	ASSERT(mMesh == nullptr);	//메쉬가 이미 등록되어 있을 시 사용 x
	ASSERT(mesh);

	mMesh = mesh;
	mSubMeshCount = mMesh->GetSubMeshCount();
	mMaterials.reserve(mSubMeshCount);
}

void MeshRenderer::SetTransform(Transform* transform)
{
	ASSERT(nullptr == mTransform);
	ASSERT(transform);

	mTransform = transform;
}

void MeshRenderer::AddMaterial(Material* material)
{
	mMaterials.emplace_back(material);
}

Material* MeshRenderer::GetMaterial(int index) const
{
	return mMaterials[index];
}

Mesh* MeshRenderer::GetMesh() const
{
	return mMesh;
}

bool MeshRenderer::IsCulled(const BoundingFrustum& frustum) const
{
	if (mTransform->IsStatic())
	{
		return false == frustum.Intersects(mTranslatedCullingBox);
	}

	auto worldMatrix = mTransform->GetWorldTransform();
	auto box = mMesh->mFrustumCullingBox;

	box.Transform(box, worldMatrix);

	return false == frustum.Intersects(box);
}

void MeshRenderer::Render(ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* pSrvHeap)
{
	mMesh->SetBuffers(pCommandList);

	for (int i = 0; i < mSubMeshCount; ++i)
	{
		mMaterials[i]->SetConstantBufferView(pCommandList, pSrvHeap);
		mMesh->RenderSubMeshes(pCommandList, i);
	}
}
