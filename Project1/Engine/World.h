#pragma once
#include <string>
#include <unordered_map>

enum class SystemId;
enum class ComponentManagerId;
class System;
class Engine;
class ComponentManager;
class ObjectManager;
class ModelResourceManager;
class VulkanEngine;

class World
{
public:

	// model filenames to load
	std::vector<std::string> modelFilenames = { "assets/giraffe.obj" };				// TODO: these will be moved to a json file and imported on creation of level
	std::vector<std::string> animatedFilenames = { "assets/astroBoy_walk_Max.dae" };						// 

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
	T* RequestManager(ComponentManagerId id);

	friend class ObjectResourceManager;

private:
	
	std::string m_name;

	// all the main component systems registered with this particular world space
	std::unordered_map<SystemId, System*> m_systems;

	std::unordered_map<ComponentManagerId, ComponentManager*> m_managers;

	ObjectManager *p_objectManager;
	ModelResourceManager *p_modelManager;		// prepares and regualtes all models associated with this world
};

template <typename T>
T* World::RequestManager(ComponentManagerId id)
{
	auto& man = m_managers.find(id);
	if (man != m_managers.end()) {

		return static_cast<T*>(m_managers[id]);
	}
	return nullptr;
}

