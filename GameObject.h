#pragma once
#include "pch.h"
 
#include "Mesh.h"
#include "Shader.h"
#include "Transform.h"
#include "Material.h"
#include "RigidBody.h"
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