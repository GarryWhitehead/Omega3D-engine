#include "Scene.h"

#include "Types/Object.h"

#include "Core/World.h"

namespace OmegaEngine
{

Scene::Scene(World& world)
{
	this->world = &world;
}

Scene::~Scene()
{
}

void ModelBuilder::buildRecursive(std::unique_ptr<GltfModel::ModelNode>& node, Object* parentObj, World& world)
{
	if (node->hasMesh())
	{
		auto& meshManager = world.getMeshManager();
		meshManager.addMesh(node->getMesh());

		// TODO: obtain these parameters from the config once it has been refactored
		parentObj->addComponent<ShadowComponent>(0.0f, 1.25f, 1.75f);
	}
	if (node->hasTransform())
	{
		auto& transManager = world.getTransManager();
		transManager->addTransform(node->getTransform(), parentObj);
	}

	if (node->hasSkin())
	{
		SkinnedComponent comp;
		comp.setIndex(node->getSkinIndex(), skinIndex);
		parentObj->addComponent<SkinnedComponent>(comp);
	}
	if (node->isJoint())
	{
		auto& transManager = world.getTransManager();
		transManager->addSkeleton(node->getJoint(), node->isSkeletonRoot(), parentObj);
	}
	if (node->hasAnimation())
	{
		auto& animManager = world.getAnimManager();
		size_t bufferIndex = node->getAnimIndex + animIndex;

		for (auto& index : node->getChannelIndices())
		{
			animManager.addAnimation(bufferIndex, parentObject);
		}
	}

	if (node->hasChildren())
	{
		for (uint32_t i = 0; i < node->childCount(); ++i)

			auto child = objectManager->createChildObject(parentObject);
		buildRecursive(node->getChildNode(i), child, matIndex, skinIndex, animIndex);
	}
}


void Scene::buildModel(GltfModel& model, World& world)
{
	// ** extract all the data from the gltf blob and add to the appropiate manager **
	// materials 
	auto& resManager = world.getResourceManager();

	for (auto& mat : model.materials)
	{
		MaterialInfo newMaterial;
		newMaterial.build(mat, world);
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
