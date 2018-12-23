#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <tuple>

#include "OEMaths/OEMaths.h"

namespace OmegaEngine
{

	// forward declerations
	class Camera;
	class CameraDataType;
	class AnimationManager;
	class MeshManager;
	class SceneManager;
	class LightManager;
	class TextureManager;
	class ObjectManager;
	struct SpaceInfo;

	
	class SceneManager
	{

	public:

		struct WorldInfo
		{
			const char* name;

			// this refers to the scene grid dimensions 
			uint32_t width;
			uint32_t height;

			// tells the scene manager the max number of spaces that can be loaded at one time. 
			// This is the stating a grid n*n - i.e. 3, 5, 7, 9, 11... Note: cannot be 1
			uint32_t gridSizeToLoad;
			uint32_t startingId;
			uint32_t totalSpaces;
		};

		SceneManager(std::vector<std::string>& filenames, CameraDataType& cameraData);
		~SceneManager();

		void setCurrentCamera(CameraDataType& camera);
		void loadSpaces();
		void createSpace(const char* filename);

	private:

		// this contains all we need to know to construct the spaces and their dimensions
		WorldInfo worldInfo;

		std::unique_ptr<LightManager> lightManager;
		std::unique_ptr<AnimationManager> animManager;
		std::unique_ptr<MeshManager> meshManager;
		std::unique_ptr<ObjectManager> objectManager;

		std::unique_ptr<Camera> currentCamera;

		// list of all gltf filenames that are associated with this world. This is linearised for faster lookup
		std::vector<SpaceInfo> spaces;
		std::vector<uint32_t> loadedSpaces;

		// let's us know whether this world has actually any spaces loaded
		bool isInit = false;
	};

}

