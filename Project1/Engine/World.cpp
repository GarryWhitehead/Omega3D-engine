#include "World.h"
#include "Engine/SceneParser.h"
#include "DataTypes/Camera.h"

#include "Managers/LightManager.h"
#include "Managers/MeshManager.h"
#include "Managers/TextureManager.h"
#include "Managers/SceneManager.h"
#include "Managers/AnimationManager.h"

namespace OmegaEngine
{
	
	World::World(std::string filename)
	{
		SceneParser parser;
		if (!parser.open(filename)) {
			throw std::runtime_error("Unable to open scene file.");
		}

		// set camera - only one camera per world at the moment
		CameraDataType cameraData;
		if (!parser.getCameraData(cameraData)) {
			throw std::runtime_error("Unable to initiliase camera.");
		}
		sceneManager->setCurrentCamera(cameraData);

		// the scenes are ordered according to their spatial representation in the world, i.e in a nXn grid. 
		parser.getWorldInfo(worldInfo);

		// get all the scene filnames (gltf format)
		std::vector<std::string> filenames;
		if (!parser.getSceneFileList(filenames)) {
			throw std::runtime_error("Error parsing gltf scene files.");
		}
		
		sceneManager = std::make_unique<SceneManager>(filenames, cameraData, worldInfo);
		assert(sceneManager != nullptr);

		// nowe init scene manager and load into memory the pre-determined number of spaces
		sceneManager->loadSpaces();
	}

	World::~World()
	{
		
	}

	void World::Update()
	{
		// Check whether new spaces need to be loaded into memory or removed - if so, do this on spertate threads
		// this depends on the max number of spaces that can be hosted on the CPU - determined by mem size
	}

	

}