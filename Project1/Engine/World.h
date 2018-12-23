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

		World(std::string filename);
		~World();

		void update();

	private:

		

		std::unique_ptr<SceneManager> sceneManager;


		
	};

}
