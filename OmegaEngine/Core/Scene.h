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
class MaterialInfo;

class Scene
{
public:
	Scene() = default;

	Scene(World& world);
	~Scene();

	// a scene isn't copyable
	Scene(Scene&) = delete;
	Scene& operator=(Scene&) = delete;

	void update();

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

	friend class Scene;

private:

	/// contains all the renderable objects associated with this scene.
	/// These have gone through visibilty checks and basically wrap the 
	/// data stored by the renderable manager which will be exposed to
	/// the renderer.
	RenderQueue renderQueue;

	/// The world this scene is assocaited with
	/// Should be safe, as the scene will be deleted before the world
	World& world;
};
}    // namespace OmegaEngine