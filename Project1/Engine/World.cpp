#include "World.h"
#include "utility/file_log.h"
#include "utility/message_handler.h"
#include "Engine/engine.h"
#include "Engine/ObjectResourceManager.h"
#include "Engine/ObjectManager.h"
#include "Systems/system.h"
#include "Systems/input_system.h"
#include "Systems/camera_system.h"
#include "Systems/GraphicsSystem.h"

#include "ComponentManagers/ComponentManager.h"
#include "ComponentManagers/PhysicsComponentManager.h"
#include "ComponentManagers/TransformComponentManager.h"
#include "ComponentManagers/MeshComponentManager.h"
#include "ComponentManagers/AnimationComponentManager.h"
#include "ComponentManagers/LightComponentManager.h"

#include "VulkanCore/VulkanEngine.h"
#include <iostream>

World::World(std::string name, MessageHandler *msg) :
	m_name(name),
	p_message(msg),
	p_objectManager(nullptr)
{
}

World::~World()
{
	Destroy();
}

void World::Generate(VulkanEngine *vkEngine)
{
	// create entity manager for this world which will keep track of all entites that are alive
	p_objectManager = new ObjectManager();

	// deserialise data into world space from binary file
	ObjectResourceManager resource(this);
	resource.LoadObjectData("assets/world_data/test_world.data");

	InitComponentManagers();

	// de-serialiase model data for this world using mesh manager
	auto p_meshManager = RequestComponentManager<MeshComponentManager>();
	p_meshManager->ImportOMFFile("assets/models/model_data.omf");
}

void World::UpdateSystems()
{
	for (auto system : m_systems) {

		system.second->Update();
	}
}

void World::RegisterSystems(std::vector<SystemId>& systemIds, Engine *engine, VulkanEngine *vkEngine)
{
	for (auto& id : systemIds) {

		switch (id) {
			case SystemId::INPUT_SYSTEM_ID:
				m_systems.insert(std::make_pair(std::type_index(typeid(InputSystem)), new InputSystem(this, p_message, engine->Window(),  Engine::SCREEN_WIDTH, Engine::SCREEN_HEIGHT)));
				break;
			case SystemId::CAMERA_SYSTEM_ID:
				m_systems.insert(std::make_pair(std::type_index(typeid(CameraSystem)), new CameraSystem(this, p_message, glm::vec3(15.0f, -45.0f, 0.0f), glm::vec3(0.0, 1.0, 0.0))));
				break;
			case SystemId::GRAPHICS_SYSTEM_ID:
				m_systems.insert(std::make_pair(std::type_index(typeid(GraphicsSystem)), new GraphicsSystem(this, p_message, vkEngine)));
				break;
		}
	}
}

std::type_index World::RegisterComponentManager(std::string id)
{
	if (id == " Physics") {
		std::type_index index(typeid(PhysicsComponentManager));
		m_managers.insert(std::make_pair(index, new PhysicsComponentManager()));
		return index;
	}
	else if (id == " Transform") {
		std::type_index index(typeid(TransformComponentManager));
		m_managers.insert(std::make_pair(index, new TransformComponentManager()));
		return index;
	}
	else if (id == " Mesh") {
		std::type_index index(typeid(MeshComponentManager));
		m_managers.insert(std::make_pair(index, new MeshComponentManager()));
		return index;
	}
	else if (id == " Animation") {
		std::type_index index(typeid(AnimationComponentManager));
		m_managers.insert(std::make_pair(index, new AnimationComponentManager()));
		return index;
	}
	else if (id == " Light") {
		std::type_index index(typeid(LightComponentManager));
		m_managers.insert(std::make_pair(index, new LightComponentManager()));
		return index;
	}
}

void World::InitComponentManagers()
{
	for (auto& manager : m_managers) {

		manager.second->Init(this, p_objectManager);
	}
}

void World::UpdateComponentManagers()
{
	for (auto& manager : m_managers) {

		manager.second->Update();
	}
}

void World::Destroy()
{
	// Destroy all systems associated with this world
	for (auto& system : m_systems) {
		system.second->Destroy();
	}
	m_systems.clear();

	// and all component managers
	for (auto& manager : m_managers) {
		manager.second->Destroy();
	}
	m_managers.clear();

	if (p_objectManager != nullptr) {
		delete p_objectManager;
	}
	p_objectManager = nullptr;
}