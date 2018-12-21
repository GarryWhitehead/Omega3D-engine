#pragma once
#include <vector>

#include "OEMaths/OEMaths.h"

namespace OmegaEngine
{

	// forward declerations
	class Node;
	class Camera;
	class CameraDataType;
	class AnimationManager;
	class MeshManager;
	class SceneManager;
	class LightManager;
	class TextureManager;

	class SceneManager
	{

	public:

		struct SceneInfo
		{
			SceneInfo(const char* name) :
				filename(name)
			{}

			const char* filename;
			bool isLoaded = false;
			bool isActive = false;
			uint32_t spaceId;
		};

		SceneManager(std::vector<std::string>& filenames, CameraDataType& cameraData);
		~SceneManager();

		void setCurrentCamera(CameraDataType& camera);

	private:

		std::unique_ptr<LightManager> lightManager;
		std::unique_ptr<TextureManager> textureManager;
		std::unique_ptr<AnimationManager> animManager;
		std::unique_ptr<MeshManager> meshManager;

		std::unique_ptr<Camera> currentCamera;

		// list of all gltf filenames that are associated with this world. Not all scenes will be loaded in at once depending on the machine specs, etc.
		std::vector<SceneInfo> sceneInfo;

		std::vector<Node> nodeBuffer;
		std::vector<Node> linearNodeBuffer;
	};

}

