#include "pch.h"
#include "GameObject.h"

void GameObject::LoadGameObjectData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12RootSignature* pRootSignature, const std::string& filePath)
{
	std::ifstream loader{ filePath, std::ios::binary };

	if (not loader)
	{
		exit(-1);
	}

	XMFLOAT3 position;
	loader.read(reinterpret_cast<char*>(&position), sizeof(XMFLOAT3));

	XMFLOAT4 rotation;
	loader.read(reinterpret_cast<char*>(&rotation), sizeof(XMFLOAT4));

	XMFLOAT3 scale;
	loader.read(reinterpret_cast<char*>(&scale), sizeof(XMFLOAT3));

	bool bHasMesh;
	loader.read(reinterpret_cast<char*>(&bHasMesh), sizeof(bool));

	mTransform.SetPosition(position);

	if (bHasMesh)
	{
		int meshLength = 0;
		char meshName[64] = {};

		loader.read(reinterpret_cast<char*>(&meshLength), sizeof(int));
		loader.read(reinterpret_cast<char*>(meshName), meshLength);
		meshName[meshLength] = '\0';
		std::string meshPath = "Assets/Models/";
		meshPath += meshName;
		meshPath += ".bin";

		mMesh.LoadMeshData(pDevice, pCommandList, meshPath);
	}

	int childCount;
	loader.read(reinterpret_cast<char*>(&childCount), sizeof(int));

	for (int i = 0; i < childCount; ++i)
	{

	}

	mShader.Initialize(pDevice, pRootSignature);
}

void GameObject::Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12RootSignature* pRootSignature)
{
	mMesh.Initialize(pDevice, pCommandList);
	mShader.Initialize(pDevice, pRootSignature);
}

void GameObject::SetTransformData(const XMFLOAT3& position, const XMFLOAT4 rotation, const XMFLOAT3& scale)
{
	mTransform.SetPosition(position);
	mTransform.SetRotationByQuat(XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w));
}

void GameObject::Render(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* pCbvHeap)
{
	mShader.SetPipelineState(pCommandList);
	mMesh.Render(pCommandList, pCbvHeap, pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	
}

Transform& GameObject::GetTransform()
{
	return mTransform;
}

