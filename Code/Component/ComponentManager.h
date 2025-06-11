#pragma once

#include "pch.h"
#include "IComponent.h"

constexpr UINT MAX_COMPONENT_SIZE = 100;

class ComponentManager final {
public:
	ComponentManager() = default;
	~ComponentManager() {
		for (auto& pair : mFlatComponents) {
			delete pair.second;
		}
	}

	ComponentManager(const ComponentManager&) = delete;

	static ComponentManager& Instance() {
		static ComponentManager instance;
		return instance;
	}

	void UpdateComponent(const float deltaTime) {
		for (auto& pair : mFlatComponents) {
			if (pair.second) {
				pair.second->Update(deltaTime);
			}
		}
	}

	template<class Component>
	void AddComponent(int objectID) {
		static unsigned int componentID = -1;
		if (componentID == -1)
			componentID = GetID<Component>(mGlobalID++);

		ASSERT(false == HasComponent<Component>(objectID));

		uint64_t key = MakeKey(objectID, componentID);
		mFlatComponents[key] = new Component();
		mObjectIDs.insert(objectID);
	}

	template<class Component>
	Component& GetComponent(int objectID) {
		static unsigned int componentID = -1;
		if (componentID == -1)
			componentID = GetID<Component>();

		uint64_t key = MakeKey(objectID, componentID);

		ASSERT(mFlatComponents.find(key) != mFlatComponents.end());

		return *(Component*)(mFlatComponents[key]);
	}

	template<class Component>
	bool HasComponent(int objectID) {
		static unsigned int componentID = -1;
		if (componentID == -1)
			componentID = GetID<Component>();

		uint64_t key = MakeKey(objectID, componentID);
		return mFlatComponents.find(key) != mFlatComponents.end();
	}


	template<typename... Components>
	std::vector<UINT> Query() {
		std::bitset<MAX_COMPONENT_SIZE> queryKey;
		std::vector<UINT> result;

		// 1. 컴포넌트 ID 추출 및 bitset 구성
		std::array<int, sizeof...(Components)> componentIDs = { GetID<Components>()... };
		for (int id : componentIDs) {
			if (id < 0 || id >= MAX_COMPONENT_SIZE) return {}; // 잘못된 ID 예외 처리
			queryKey.set(id);
		}

		// 2. 캐시 확인
		if (mObjectGroups.find(queryKey) != mObjectGroups.end()) {
			return mObjectGroups[queryKey];
		}

		for (const auto& objectID : mObjectIDs) {
			if (HasAllComponents<Components...>(objectID)) {
				result.push_back(objectID);
			}
		}

		mObjectGroups[queryKey] = result;
		return result;
	}
private:
	template<class Component>
	int GetID(int id = -1) {
		static int componentID = -1;
		if (componentID == -1 && id != -1)
			componentID = id;

		return componentID;
	}

	inline uint64_t MakeKey(unsigned int objectID, unsigned int componentID) const {
		return (static_cast<uint64_t>(componentID) << 32) | static_cast<uint64_t>(objectID);
	}

	template<typename First, typename... Rest>
	bool HasAllComponents(UINT objectID) {
		if (!HasComponent<First>(objectID)) return false;
		if constexpr (sizeof...(Rest) == 0)
			return true;
		else
			return HasAllComponents<Rest...>(objectID);
	}
private:
	unsigned int mGlobalID = 0;
	std::set<UINT> mObjectIDs;
	std::unordered_map<uint64_t, IComponent*> mFlatComponents; // key = (objectID << 32) | componentID
	std::unordered_map<std::bitset<MAX_COMPONENT_SIZE>, std::vector<UINT>> mObjectGroups;
};

//#pragma once
//
//#include "pch.h"
//#include "IComponent.h"
//
//class ComponentManager final {
//public:
//	ComponentManager() = default;
//	~ComponentManager() {
//		for (auto object : mComponents)
//		{
//			for (auto component : object.second)
//			{
//				delete component.second;
//			}
//		}
//	}
//
//	ComponentManager(const ComponentManager&) = delete;
//
//	static ComponentManager& Instance() {
//		static ComponentManager componentMangager;
//		return componentMangager;
//	}
//
//	void UpdateComponent(const float deltaTime)
//	{
//		for (auto& Components : mComponents)
//		{
//			for (auto& Component : Components.second)
//			{
//				IComponent* component = Component.second;
//				component->Update(deltaTime);
//			}
//		}
//	}
//
//	template<class Component>
//	void AddComponent(int objectID) {
//		static unsigned int componentID = -1;
//
//		if (componentID == -1)
//		{
//			componentID = GetID<Component>(mGlobalID++);
//		}
//
//		ASSERT(false == HasComponent<Component>(objectID));
//
//		mComponents[objectID][componentID] = new Component();
//	};
//
//	template<class Component>
//	int GetID(int id = -1)
//	{
//		static int componentID = -1;
//
//		if (componentID == -1)
//		{
//			componentID = id;
//		}
//
//		return componentID;
//	}
//
//	template<class Component>
//	Component& GetComponent(int objectID)
//	{
//		static unsigned int componentID = -1;
//
//		ASSERT(true == HasComponent<Component>(objectID));
//
//		Component& component = *(Component*)(mComponents[objectID][GetID<Component>()]);
//
//		return component;
//	}
//
//	template<class Component>
//	bool HasComponent(int objectID)
//	{
//		return (mComponents[objectID].find(GetID<Component>()) != mComponents[objectID].end());
//	}
//
//private:
//	unsigned int mGlobalID = 0;
//
//	std::unordered_map<unsigned int, std::unordered_map<unsigned int, IComponent*>> mComponents;	// ObjectID , ComponentID, Pointer
//};