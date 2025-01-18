#pragma once
#include "pch.h"
 
#include "Mesh.h"
#include "Shader.h"
#include "Transform.h"
#include "d3dUtil.h"
#include "ComponentManager.h"

class GameObject {
public:
	struct ObjectConstants {
		XMFLOAT4X4 WorldViewProj = {};
	};

	GameObject();
	~GameObject() {}

	//GameObject(const GameObject& rhs) = delete;

	void LoadGameObjectData(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12RootSignature* pRootSignature, const std::string& filePath);

	void Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12RootSignature* pRootSignature);

	void SetTransformData(const XMFLOAT3& position, const XMFLOAT4 rotation, const XMFLOAT3& scale);
	void SetMesh();

	void Render(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* pCbvHeap);

	Transform& GetTransform();

	Mesh mMesh = {};
	Shader mShader = {};

	template <class Component>
	void AddComponent()
	{
		ComponentManager::Instance().AddComponent<Component>(ID);
	}
private:
	static unsigned int globalID;

	Transform mTransform;

	float speed = 0.0f;

	unsigned int ID = 0;
};