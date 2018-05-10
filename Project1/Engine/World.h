#pragma once
#include <string>
#include <unordered_map>

enum class SystemId;
enum class ComponentManagerId;
class System;
class Engine;
class ComponentManager;
class ObjectManager;
class VulkanEngine;

class World
{
public:

	World();
	World(std::string name);
	~World();

	void Generate(VulkanEngine *engine);

	// main system functions
	void UpdateSystems();
	System* HasSystem(SystemId id);
	void RegisterSystems(std::vector<SystemId>& systemIds,  Engine *engine, VulkanEngine *vkEngine);
	void UpdateComponentManagers();
	void Destroy();

	// component manager functions
	void World::InitComponentManagers();
	void RegisterComponentManager(ComponentManagerId id);
	void LinkComponentManager(ComponentManagerId srcId, ComponentManagerId dstId);
	void LinkManagerWithSystem(ComponentManagerId m_id, SystemId sysId);

	template <typename T>
	T* RequestComponentManager(ComponentManagerId id);

	friend class ObjectResourceManager;

private:
	
	std::string m_name;

	// all the main component systems registered with this particular world space
	std::unordered_map<SystemId, System*> m_systems;

	std::unordered_map<ComponentManagerId, ComponentManager*> m_managers;

	ObjectManager *p_objectManager;
};

template <typename T>
T* World::RequestComponentManager(ComponentManagerId id)
{
	auto& man = m_managers.find(id);
	if (man != m_managers.end()) {

		return static_cast<T*>(m_managers[id]);
	}
	return nullptr;
}

