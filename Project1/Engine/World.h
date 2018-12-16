#pragma once
#include <string>
#include <unordered_map>
#include <map>
#include <typeindex>

// forward declerartions
enum class SystemId;
class System;
class Engine;
class ComponentManager;
class ObjectManager;
class VulkanEngine;
class MessageHandler;

class World
{
public:

	World(std::string filename, std::string worldName);
	~World();

	void Generate(VulkanEngine *engine);

	// main system functions
	void UpdateSystems();
	void RegisterSystems(std::vector<SystemId>& systemIds,  Engine *engine, VulkanEngine *vkEngine);
	void UpdateComponentManagers();
	void Destroy();

	// component manager functions
	void World::InitComponentManagers();
	std::type_index RegisterComponentManager(std::string id);

	template <typename T>
	bool HasSystem();

	template <typename T>
	T *RequestSystem();

	template <typename T>
	T* RequestComponentManager();

	friend class ObjectResourceManager;

private:
	
	std::string name;

	// all the main component systems registered with this particular world space
	std::unordered_map<std::type_index, System*> m_systems;

	std::unordered_map<std::type_index, ComponentManager*> m_managers;
	
};

template <typename T>
T* World::RequestComponentManager()
{
	std::type_index index(typeid(T));

	auto& manager = m_managers.find(index);
	if (manager != m_managers.end()) {

		return static_cast<T*>(m_managers[index]);
	}

	return nullptr;
}

template <typename T>
T* World::RequestSystem()
{
	std::type_index index(typeid(T));

	auto& sys = m_systems.find(index);
	if (sys != m_systems.end()) {

		return static_cast<T*>(m_systems[index]);
	}
	return nullptr;
}

template <typename T>
bool World::HasSystem()
{
	std::type_index index(typeid(T));

	auto& sys = m_systems.find(index);
	return (sys != m_systems.end());
}

