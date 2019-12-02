#pragma once

#include "Core/ObjectManager.h"

#include "Models/Formats/GltfModel.h"

#include "Rendering/RenderQueue.h"

#include <vector>

namespace OmegaEngine
{

// forward decleartions
class World;
class GltfModel;
class Object;
class Engine;

class Scene
{
public:
	Scene() = default;

	Scene(World& world, Engine& engine);
	~Scene();

	// a scene isn't copyable
	Scene(Scene&) = delete;
	Scene& operator=(Scene&) = delete;

	void update();

	void prepare();
    
    void updateCamera();

	/**
	* Return an instance of the object manager
	* Used by the user for creating objects - usually passed to the scene
	*/
	ObjectManager& getObjManager()
	{
		return objManager;
	}

	friend class Renderer;
    
private:

	/// Current cameras associated with this scene.
    std::vector<Camera> cameras;
    uint16_t currCamera = UINT16_MAX;
    
	/// The world this scene is assocaited with
	World& world;

	Engine& engine;
};
}    // namespace OmegaEngine
