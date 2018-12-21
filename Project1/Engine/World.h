#pragma once
#include <string>
#include <vector>

namespace OmegaEngine
{
	// forward declerartions
	class Camera;

	class World
	{
	public:

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

		void update();

	private:

		WorldInfo worldInfo;

		std::unique_ptr<SceneManager> sceneManager;


		
	};

}
