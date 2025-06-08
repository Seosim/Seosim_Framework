#pragma once
#include "pch.h"
 
#include "Resource/Mesh.h"
#include "Resource/Shader/Shader.h"
#include "Resource/Shader/ComputeShader.h"
#include "Component/Transform.h"
#include "Resource/Material.h"
#include "Component/RigidBody.h"
#include "Component/MeshRenderer.h"
#include "Component/Collider/BoxCollider.h"
#include "Component/Collider/TerrainMeshCollider.h"
#include "Component/PlayerController.h"
#include "Component/CameraController.h"
#include "d3dUtil.h"
#include "Component/ComponentManager.h"

class GameObject {
public:
	GameObject();
	GameObject(const std::string& name);
	~GameObject() {}

	GameObject(const GameObject& rhs) = delete;

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
	std::string mName = {"Default_Name"};
};