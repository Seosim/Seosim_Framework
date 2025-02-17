#pragma once
#include "pch.h"
 
#include "Mesh.h"
#include "Shader.h"
#include "Transform.h"
#include "Material.h"
#include "d3dUtil.h"
#include "ComponentManager.h"

class GameObject {
public:
	struct ObjectConstants {
		XMFLOAT4X4 WorldViewProj = {};
	};

	GameObject();
	~GameObject() {}

	GameObject(const GameObject& rhs) = delete;

	void Initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12RootSignature* pRootSignature);

	void Render(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12DescriptorHeap* pCbvHeap);

	template <class Component>
	void AddComponent()
	{
		ComponentManager::Instance().AddComponent<Component>(ID);
	}

	template<class Component>
	Component& GetComponent()
	{
		return ComponentManager::Instance().GetComponent<Component>(ID);
	}

	template<class Component>
	bool HasComponent()
	{
		return ComponentManager::Instance().HasComponent<Component>(ID);
	}
private:
	static unsigned int globalID;

	unsigned int ID = 0;
};