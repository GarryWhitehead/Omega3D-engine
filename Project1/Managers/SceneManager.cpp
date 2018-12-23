#include "SceneManager.h"
#include "DataTypes/Camera.h"
#include "Utility/GeneralUtil.h"
#include "Threading/ThreadPool.h"
#include "Engine/GltfParser.h"
#include "Omega_Common.h"

namespace OmegaEngine
{

	SceneManager::SceneManager(std::vector<std::string>& filenames, CameraDataType& cameraData)
	{
		lightManager = std::make_unique<LightManager>();
		meshManager = std::make_unique<MeshManager>();
		animManager = std::make_unique<AnimationManager>();
		textureManager = std::make_unique<TextureManager>();

		assert(lightManager != nullptr);
		assert(meshManager != nullptr);
		assert(animManager != nullptr);

		// init scene information
		for (auto& filename : filenames) {
			sceneInfo.push_back({ filename.c_str() });
		}

		// set the current camera - only one supported at present
		currentCamera = std::make_unique<Camera>(cameraData);
	}


	SceneManager::~SceneManager()
	{
	}

	void SceneManager::setCurrentCamera(CameraDataType& cameraInfo)
	{
		currentCamera = std::make_unique<Camera>(cameraInfo);
		assert(currentCamera != nullptr);
	}

	void SceneManager::loadSpaces()
	{
		// if no spaces are loaded then we don't need to worry about position of camera, etc.
		if (loadedSpaces.empty()) {

			// calculate how many spaces to load depending on the start position and max spaces to load
			assert(worldInfo.gridSizeToLoad > 1);

			uint32_t min, max;
			uint32_t gridWidth, gridHeight;
			if (Util::min_max_1dto2dgrid(worldInfo.startingId, worldInfo.width, worldInfo.gridSizeToLoad, gridWidth, gridHeight, min, max)) {
				throw std::runtime_error("Error whilst calculating grid sizes. Porbably due to incorrectly specified grid size to load");
			}

#ifdef OMEGA_ENGINE_THREADED
			// we can now load all the required spaces on background threads
			ThreadPool threadPool(ASSET_LOAD_THREAD_COUNT);

			for (uint32_t y = min; y < gridHeight; ++y) {
				for (uint32_t x = min; x < gridWidth; ++x) {
					threadPool.submitTask([&x, &y, &gridWidth, this]() {
						createSpace(spaces[y * gridWidth + x].filename);
					});
				}
			}

#else
			for (uint32_t y = min; y < gridWidth; ++y) {
				for (uint32_t x = min; x < gridHeight; ++x) {
					loadGltfSpace(spaces[y * gridWidth + x].filename);
				}
			}
#endif
		}
	}

	void SceneManager::createSpace(const char* filename)
	{
		GltfParser parser;

		if (!parser.parse(filename)) {
			throw std::runtime_error("Error whilst trying to open Gltf file.");
		}

		// Add the parsed data to the appropiate managers. This will automatically create the objects and vulkan-fy things ready for rendering
		meshManager->addData(parser.getMeshData(), parser.getIndiciesData(), objectManager);
	}

	
}