#include "World.h"
#include "utility/file_log.h"
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

World::World() :
	p_objectManager(nullptr)
{
}

World::World(std::string name) : 
	m_name(name),
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
	auto p_meshManager = RequestComponentManager<MeshComponentManager>(ComponentManagerId::CM_MESH_ID);
	p_meshManager->ImportOMFFile("assets/models/model_data.omf");
}

void World::UpdateSystems()
{
	for (auto system : m_systems) {

		system.second->Update();
	}
}

System* World::HasSystem(SystemId id)
{
	auto& sys = m_systems.find(id);
	if (sys != m_systems.end()) {
		return m_systems[id];
	}
	return nullptr;
}

void World::RegisterSystems(std::vector<SystemId>& systemIds, Engine *engine, VulkanEngine *vkEngine)
{
	for (auto& id : systemIds) {

		if (id == SystemId::INPUT_SYSTEM_ID) {

			InputSystem *system = new InputSystem(this);
			system->Init(engine->Window(), static_cast<CameraSystem*>(m_systems[SystemId::CAMERA_SYSTEM_ID]), Engine::SCREEN_WIDTH, Engine::SCREEN_HEIGHT);
			m_systems.insert(std::make_pair(SystemId::INPUT_SYSTEM_ID, system));
		}
		else if(id == SystemId::CAMERA_SYSTEM_ID) {

			CameraSystem *system = new CameraSystem(this);
			system->Init(glm::vec3(15.0f, -45.0f, 0.0f));
			m_systems.insert(std::make_pair(SystemId::CAMERA_SYSTEM_ID, system));

			// setup camera view
			system->SetPerspective(Engine::CAMERA_FOV, static_cast<float>(Engine::SCREEN_WIDTH / Engine::SCREEN_HEIGHT), 0.1f, 512.0f);
		}
		else if (id == SystemId::GRAPHICS_SYSTEM_ID) {

			GraphicsSystem *system = new GraphicsSystem(this, vkEngine);
			system->Init();
			m_systems.insert(std::make_pair(SystemId::GRAPHICS_SYSTEM_ID, system));
		}
		else {
			*g_filelog << "Error registering engine system with id #" << (int)id << ". Engine system id not recognised.\n";
		}
	}
}

void World::RegisterComponentManager(ComponentManagerId id)
{
	
	if (id == ComponentManagerId::CM_PHYSICS_ID) {

		PhysicsComponentManager *manager = new PhysicsComponentManager(ComponentManagerId::CM_PHYSICS_ID);
		m_managers.insert(std::make_pair(ComponentManagerId::CM_PHYSICS_ID, manager));
	}
	else if (id == ComponentManagerId::CM_TRANSFORM_ID) {

		TransformComponentManager *manager = new TransformComponentManager(ComponentManagerId::CM_TRANSFORM_ID);
		m_managers.insert(std::make_pair(ComponentManagerId::CM_TRANSFORM_ID, manager));
	}
	else if (id == ComponentManagerId::CM_MESH_ID) {

		MeshComponentManager *manager = new MeshComponentManager(ComponentManagerId::CM_MESH_ID);
		m_managers.insert(std::make_pair(ComponentManagerId::CM_MESH_ID, manager));
	}
	else if (id == ComponentManagerId::CM_ANIMATION_ID) {

		AnimationComponentManager *manager = new AnimationComponentManager(ComponentManagerId::CM_ANIMATION_ID);
		m_managers.insert(std::make_pair(ComponentManagerId::CM_ANIMATION_ID, manager));
	}
	else if (id == ComponentManagerId::CM_LIGHT_ID) {

		LightComponentManager *manager = new LightComponentManager(ComponentManagerId::CM_LIGHT_ID);
		m_managers.insert(std::make_pair(ComponentManagerId::CM_LIGHT_ID, manager));
	}
	else {
		*g_filelog << "Error registering component manager with id #" << (int)id << ". Component manager id not recognised.\n";
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

void World::LinkComponentManager(ComponentManagerId srcId, ComponentManagerId dstId)
{
	m_managers[dstId]->RegisterManager(m_managers[srcId]);
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