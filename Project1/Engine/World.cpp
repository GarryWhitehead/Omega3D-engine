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
		// init all the managers required to generate the world
		initComponentManagers();
		
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
		for (auto& filename : filenames) {
			sceneInfo.push_back({ filename.c_str() });
		}
	}

	World::~World()
	{
		
	}

	void World::initComponentManagers()
	{
		lightManager = std::make_unique<LightManager>();
		meshManager = std::make_unique<MeshManager>();
		animManager = std::make_unique<AnimationManager>();
		sceneManager = std::make_unique<SceneManager>();
		textureManager = std::make_unique<TextureManager>();

		assert(lightManager != nullptr);
		assert(meshManager != nullptr);
		assert(animManager != nullptr);
		assert(sceneManager != nullptr);
		assert(textureManager != nullptr);
	}

	void World::Update()
	{

	}

	

}