#include "World.h"
#include "Engine/Omega_SceneParser.h"
#include "DataTypes/Camera.h"
#include "ComponentSystem/ComponentSystem.h"
#include "Managers/LightManager.h"
#include "Managers/MeshManager.h"
#include "Managers/TextureManager.h"
#include "Managers/SceneManager.h"
#include "Managers/AnimationManager.h"

namespace OmegaEngine
{
	
	World::World(Managers managers)
	{
		compSystem = std::make_unique<ComponentSystem>();
		
		// register all components managers required for this world
		if (managers & Managers::OE_MANAGERS_MESH || managers & Managers::OE_MANAGERS_ALL) {
			compSystem->registerManager<MeshManager>();
		}
		if (managers & Managers::OE_MANAGERS_ANIMATION || managers & Managers::OE_MANAGERS_ALL) {
			compSystem->registerManager<AnimationManager>();
		}
		if (managers & Managers::OE_MANAGERS_LIGHT || managers & Managers::OE_MANAGERS_ALL) {
			compSystem->registerManager<LightManager>();
		}
	}

	World::~World()
	{
		
	}

	bool World::create(const char* filename)
	{
		SceneParser parser;
		if (!parser.parse(filename)) {
			throw std::runtime_error("Unable to parse omega engine scene file.");
		}

		// add the data parsed from the scene file to the appropiate managers
		auto& meshManager = compSystem->getManager<MeshManager>();
		meshManager->addData();

	}

	void World::update()
	{
		// Check whether new spaces need to be loaded into memory or removed - if so, do this on spertate threads
		// this depends on the max number of spaces that can be hosted on the CPU - determined by mem size
	}



	

}