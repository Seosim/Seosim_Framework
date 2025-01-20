#pragma once

#include "pch.h"

#include "IComponent.h"

class ComponentManager final {
public:
	ComponentManager() = default;
	~ComponentManager() {
		for (auto object : mComponents)
		{
			for (auto component : object.second)
			{
				delete component.second;
			}
		}
	}

	ComponentManager(const ComponentManager&) = delete;

	static ComponentManager& Instance() {
		static ComponentManager componentMangager;
		return componentMangager;
	}

	template<class Component>
	void AddComponent(int objectID) {
		static unsigned int componentID = -1;

		if (componentID == -1)
		{
			componentID = GetID<Component>(mGlobalID++);

		}

		mComponents[objectID][componentID] = new Component();
	};

	template<class Component>
	int GetID(int id = -1)
	{
		static int componentID = -1;

		if (componentID == -1)
		{
			componentID = id;
		}

		return componentID;
	}

	template<class Component>
	Component& GetComponent(int objectID)
	{
		static unsigned int componentID = -1;

		ASSERT(mComponents[objectID][GetID<Component>()] != nullptr);

		Component& component = *(Component*)(mComponents[objectID][GetID<Component>()]);

		return component;
	}

	template<class Component>
	bool HasComponent(int objectID)
	{
		return (mComponents[objectID][GetID<Component>()] != nullptr);
	}

private:
	unsigned int mGlobalID = 0;

	std::unordered_map<unsigned int, std::unordered_map<unsigned int, IComponent*>> mComponents;	// ObjectID , ComponentID, Pointer
};


