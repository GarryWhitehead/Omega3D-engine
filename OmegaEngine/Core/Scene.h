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

	/**
	* Return an instance of the object manager
	* Used by the user for creating objects - usually passed to the scene
	*/
	ObjectManager& getObjManager()
	{
		return objManager;
	}

private:
	// private functions to the scene

	/**
	* Adds a renderable type to the list.
	* @param T renderable type
	* @param args argument list which matches the init list for this particular renderable
	*/
	template <typename T, typename... Args>
	void addRenderable(Args&&... args)
	{
		T* renderable = new T(std::forward<Args>(args)...);
		renderables.push_back({ renderable });
	}

	friend class Renderer;
    
private:

	/// Current camera associated with this scene. Updated by the camera manager
    Camera camera;
    
	/// The world this scene is assocaited with
	World& world;

	Engine& engine;
};
}    // namespace OmegaEngine
