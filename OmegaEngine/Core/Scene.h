#pragma once

#include "Types/Object.h"

#include "Core/World.h"

#include "Managers/ObjectManager.h"

#include <vector>

namespace OmegaEngine
{

class Scene
{
public:

	Scene(World& world);
	~Scene();

	void buildRenderableMeshTree(Object& obj);
	void update();

	/**
	* Gets a renderable type from the list
	* @param index the index of the renderable type
	* @return A renderable type - this will be as the base class
	*/
	RenderableInfo& getRenderable(uint32_t index)
	{
		assert(index < renderables.size());
		return renderables[index];
	}

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

private:

	/// objects assoicated with this scene dealt with by the object manager
	ObjectManager objManager;

	/// contains all the renderable objects associated with this scene.
	/// This data is used by the renderer to render the scene to the surface 
	std::vector<RenderableBase*> renderables;
	
	/// The world this scene is assocaited with
	/// Should be safe, as the scene will be deleted before the world
	World& world;

};
}    // namespace OmegaEngine