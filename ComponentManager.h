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

	void UpdateComponent(const float deltaTime)
	{
		for (auto& Components : mComponents)
		{
			for (auto& Component : Components.second)
			{
				IComponent* component = Component.second;
				component->Update(deltaTime);
			}
		}
	}

	template<class Component>
	void AddComponent(int objectID) {
		static unsigned int componentID = -1;

		if (componentID == -1)
		{
			componentID = GetID<Component>(mGlobalID++);
		}

		ASSERT(false == HasComponent<Component>(objectID));

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

		ASSERT(true == HasComponent<Component>(objectID));

		Component& component = *(Component*)(mComponents[objectID][GetID<Component>()]);

		return component;
	}

	template<class Component>
	bool HasComponent(int objectID)
	{
		return (mComponents[objectID].find(GetID<Component>()) != mComponents[objectID].end());
	}

private:
	unsigned int mGlobalID = 0;

	std::unordered_map<unsigned int, std::unordered_map<unsigned int, IComponent*>> mComponents;	// ObjectID , ComponentID, Pointer
};