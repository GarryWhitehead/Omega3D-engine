#include "Scene.h"

#include "Types/Object.h"

#include "Core/World.h"

#include "Components/RenderableManager.h"

namespace OmegaEngine
{

Scene::Scene(World& world)
{
	this->world = &world;
}

Scene::~Scene()
{
}

void Scene::buildModel(GltfModel& model, World& world)
{
	auto& rendManager = world.getRendManager();
	auto& transManager = world.getTransManager();

	// first add the materials to the renderable manager. We need the buffer offset for the primitives
	size_t matOffset = rendManager.addMaterial(model.materials.data(), model.materials.size());

	for (auto& node : model.nodes)
	{
		// create an object for each mesh
		Object* obj = objManager.createObject();
		rendManager.addMesh(node.mesh, *obj, matOffset);

		// send the transforms and the node hierachy to the transform manager
		transManager.addNodeHierachy(node, *obj);
	}

	// skins
	auto& transManager = world.getTransManager();
	for (auto& skin : model.skins)
	{
		transManager->addSkin(skin);
	}

	// animations
	auto& animManager = world.getAnimManager();
	for (auto& anim : model.animations)
	{
		animManager.addAnim(anim);
	}

	// go through each node and add as a child of the parent
	for (auto& node : model.nodes)
	{
		
		Object child = ObjectManager::createObject();

		buildRecursive(node, child, world);
		obj.addChild(child);
	}
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
