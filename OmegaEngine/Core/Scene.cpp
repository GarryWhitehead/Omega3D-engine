#include "Scene.h"

namespace OmegaEngine
{

Scene::Scene(World& world)
{
	this->world = &world;
}

Scene::~Scene()
{
}

void Scene::buildRenderableMeshTree(Object& obj)
{
	auto& meshManager = world.getMeshManager();
	auto& lightManager = world.getLightManager();

	if (obj.hasComponent<MeshComponent>())
	{
		auto& mesh = meshManager.getMesh(obj.getComponent<MeshComponent>());

		// we need to add all the primitve sub meshes as renderables
		for (auto& primitive : mesh.primitives)
		{
			addRenderable<RenderableMesh>(componentInterface, vkInterface, mesh, primitive, obj, stateManager,
			                              renderer);

			// if using shadows, then draw the meshes into the offscreen depth buffer too
			if (obj.hasComponent<ShadowComponent>())
			{
				addRenderable<RenderableShadow>(stateManager, vkInterface, obj.getComponent<ShadowComponent>(), mesh,
				                                primitive, lightManager.getLightCount(),
				                                lightManager.getAlignmentSize(), renderer);
			}
		}
	}

	// check whether the child objects contain mesh data
	auto children = obj.getChildren();
	for (auto child : children)
	{
		buildRenderableMeshTree(child);
	}
}

void Scene::update()
{
	if (isDirty)
	{
		for (auto& object : objects)
		{
			// objects which have skybox, landscape or ocean components won't have other components checked
			if (object.hasComponent<SkyboxComponent>())
			{
				addRenderable<RenderableSkybox>(stateManager, object.getComponent<SkyboxComponent>(), vkInterface,
				                                renderer);
			}
			else
			{
				// check whether object or children have mesh data
				buildRenderableMeshTree(object);
			}
		}

		isDirty = false;
	}
}

}    // namespace OmegaEngine
