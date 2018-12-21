#pragma once
#include <string>
#include <vector>

namespace OmegaEngine
{
	// forward declerartions
	class Camera;
	class AnimationManager;
	class MeshManager;
	class SceneManager;
	class LightManager;
	class TextureManager;

	class World
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

		struct WorldInfo
		{
			const char* name;
			
			// this refers to the scene grid dimensions 
			uint32_t width;
			uint32_t height;

			uint32_t totalSpaces;
		};

		World(std::string filename);
		~World();

		void initComponentManagers();

		void update();

	private:

		WorldInfo worldInfo;

		// maangers
		std::unique_ptr<AnimationManager> animManager;
		std::unique_ptr<MeshManager> meshManager;
		std::unique_ptr<SceneManager> sceneManager;
		std::unique_ptr<LightManager> lightManager;
		std::unique_ptr<TextureManager> textureManager;

		// list of all gltf filenames that are associated with this world. Not all scenes will be loaded in at once depending on the machine specs, etc.
		std::vector<SceneInfo> sceneInfo;
	};

}
