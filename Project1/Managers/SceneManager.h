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
	struct Mesh;

	
	class SceneManager
	{

	public:

		struct ModelNode
		{
			// index to child nodes
			std::vector<uint32_t> children;		
			uint32_t meshIndex;

			OEMaths::mat4f local_transform;
		};

		struct Model
		{
			uint32_t verticesOffset;
			uint32_t indicesOffset;
			
			ModelNode node;
			
			OEMaths::mat4f world_transform;

			// vulkan stuff
		};

		SceneManager(std::vector<std::string>& filenames, CameraDataType& cameraData);
		~SceneManager();

		void setCurrentCamera(CameraDataType& camera);
		void loadSpaces();
		void createSpace(const char* filename);

	private:

		std::unique_ptr<LightManager> lightManager;
		std::unique_ptr<AnimationManager> animManager;
		std::unique_ptr<MeshManager> meshManager;
		std::unique_ptr<ObjectManager> objectManager;

		std::unique_ptr<Camera> currentCamera;

		// list of all gltf filenames that are associated with this world. This is linearised for faster lookup
		std::vector<Model> models;


		// let's us know whether this world has actually any spaces loaded
		bool isInit = false;
	};

}

