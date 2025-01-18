#pragma once

#include "pch.h"

#include "IComponent.h"

class ComponentManager final {
public:
	ComponentManager() = default;
	~ComponentManager() {}

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
			componentID = mGlobalID++;
		}

		mComponents[objectID][componentID] = new Component();
	};
private:
	unsigned int mGlobalID = 0;

	std::unordered_map<unsigned int, std::unordered_map<unsigned int, IComponent*>> mComponents;
};


