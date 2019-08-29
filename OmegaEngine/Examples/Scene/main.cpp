#pragma once

#include "Engine/engine.h"
#include "Engine/world.h"
#include "Managers/CameraManager.h"
#include "Managers/LightManager.h"
#include "Managers/TransformManager.h"
#include "Models/Gltf/GltfModel.h"
#include "Models/OEMaterials.h"
#include "Models/OEModels.h"
#include "ObjectInterface/ComponentTypes.h"
#include "ObjectInterface/Object.h"

// An example of building a scene using the component-object interface.
// Very much a work in progress at the moment.

using namespace OmegaEngine;

void extractGltfModelAssets(std::unique_ptr<GltfModel::Model>& model, World* world, uint32_t& matIndex,
                                   uint32_t& skinIndex, uint32_t& animIndex)
{
	// materials and their textures
	auto& matManager = world->getMatManager();
	matIndex = matManager->getIndex();

	for (auto& mat : model->materials)
	{
		matManager->addMaterial(mat, model->images);
	}

	// skins
	auto& transManager = world->getTransManager();
	skinIndex = transManager->getSkinIndex();

	for (auto& skin : model->skins)
	{
		transManager->addSkin(skin);
	}

	// animations
	auto& animManager = world->getAnimManager();
	animIndex = animManager->getIndex());

	for (auto& anim : model->animations)
	{
		animManager->addAnim(anim);
	}
}

void createGltfModelObjectRecursive(std::unique_ptr<GltfModel::ModelNode>& node, Object* parentObject,
                                           const uint32_t materialOffset, const uint32_t skinOffset,
                                           const uint32_t animationOffset)
{
	if (node->hasMesh())
	{
		parentObject->addComponent<MeshComponent>(node->getMesh(), materialOffset);
		// TODO: obtain these parameters from the config once it has been refactored
		parentObject->addComponent<ShadowComponent>(0.0f, 1.25f, 1.75f);
	}
	if (node->hasTransform())
	{
		parentObject->addComponent<TransformComponent>(node->getTransform());
	}
	if (node->hasSkin())
	{
		parentObject->addComponent<SkinnedComponent>(node->getSkinIndex(), skinOffset);
	}
	if (node->isJoint())
	{
		parentObject->addComponent<SkeletonComponent>(node->getJoint(), skinOffset, node->isSkeletonRoot());
	}
	if (node->hasAnimation())
	{
		parentObject->addComponent<AnimationComponent>(node->getAnimIndex(), node->getChannelIndices(),
		                                               animationOffset);
	}

	if (node->hasChildren())
	{
		for (uint32_t i = 0; i < node->childCount(); ++i)
		{
			auto child = objectManager->createChildObject(*parentObject);
			this->createGltfModelObjectRecursive(node->getChildNode(i), child, materialOffset, skinOffset,
			                                     animationOffset);
		}
	}
}

Object* build(std::unique_ptr<GltfModel::Model>& model, const OEMaths::vec3f& position,
                                     const OEMaths::vec3f& scale, const OEMaths::quatf& rotation, bool useMaterial)
{
	uint32_t materialOffset, skinOffset, animationOffset;

	extractGltfModelAssets(model, materialOffset, skinOffset, animationOffset);

	for (auto& node : model->nodes)
	{
		auto child = objectManager->createChildObject(*rootObject);

		this->createGltfModelObjectRecursive(node, child, materialOffset, skinOffset, animationOffset);
	}

	return rootObject;
}

int main(int argc, char* argv[])
{
	std::unique_ptr<Engine> engine = Engine::Init("Scene Example", 1280, 700);
	
	// create a new empty world
	std::unique_ptr<World> world = engine->createWorld("SceneOne");

	// Use a gltf image as one of our scene objects
	auto model = GltfModel::CreateInstance("WaterBottle/WaterBottle.gltf");
	model.worldTransform(4.0f, 3.0f, 5.0);
	model.worldScale(15.0f);
	model.worldRotation(0.5, 0.0f, 0.5f, 0.5f);
	model.build(world, obj);
	world->addObject(obj);

	/*// Use a gltf image as one of our scene objects
	auto model = GltfModel::load("DamagedHelmet/DamagedHelmet.gltf");

		// create an object, using a gltf model for vertices, materials, etc.	
		auto object = world->createGltfModelObject(model, OEMaths::vec3f{ 0.0f, 3.0f, 0.0f }, OEMaths::vec3f{ 2.0f },
		                                           OEMaths::quatf{ 0.0f }, true);
	}*/

	// adding stock models to the scene - add a capsule
	/*{
		auto object =
		    world->createObject(OEMaths::vec3f{ -2.0f, 0.0f, 0.0f }, OEMaths::vec3f{ 1.0f }, OEMaths::quatf{ 0.0f });

		object->addComponent<MeshComponent>(OEModels::generateCapsuleMesh(30, 5.0f, 0.5f));
		object->addComponent<MaterialComponent>("DemoMaterial", OEMaterials::Specular::Gold, OEMaths::vec3f{ 0.3f },
		                                        OEMaths::vec4f{ 0.8f, 0.2f, 0.0f, 1.0f }, 0.2f, 1.0f);
		object->addComponent<TransformComponent>(
		    TransformManager::transform(OEMaths::vec3f{ 0.0f }, OEMaths::vec3f{ 1.0f }, OEMaths::quatf{ 0.0f }));
	}*/

	// create a flat plane
	{
		Object obj = ObjectManager::create(OEMaths::vec3f{ -0.2f, 0.0f, 0.0f }, OEMaths::vec3f{ 1.0f }, OEMaths::quatf{ 0.0f });
		
		auto model = Model::BuildStock();
		model.transform(-0.2f, 0.0f, 0.0f);
		model.type(Model::Stock::Plane);
		model.material("DemoRough", OEMaterials::Specular::Titanium, OEMaths::vec3f{ 0.3f },
												 OEMaths::vec4f{ 0.0f, 0.6f, 0.8f, 1.0f }, 0.8f, 0.2f));

		auto mat = MatManager::create();
		object2->addComponent<MeshComponent>(OEModels::generateQuadMesh(0.5f));
		object2->addComponent<MaterialComponent>(;
		object2->addComponent<TransformComponent>(TransformManager::transform(
			OEMaths::vec3f{ 0.0f }, OEMaths::vec3f{ 25.0f }, OEMaths::quatf{ 0.5f, -0.5f, 0.5f, -0.5f }));
	}

	// add a skybox
	world->addSkybox("skybox/cubemap.ktx", 0.5f);

	// and a default camera - multiple cameras can be added (TODO: switch via a designated key)
	world->addCameraToWorld();

	// add different lights
	world->addSpotLightToWorld({ 0.0f, 3.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f }, 100.0f, 1000.0f, 10.0f, 0.5f, 0.5f);
	world->addDirectionalLightToWorld({ 0.0f, 0.0f, 0.0f }, { 8.0f, 8.0f, 0.0f }, { 0.9f, 0.8f, 0.2f }, 100.0f, 10000.0f);
	world->addPointLightToWorld({ 3.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f }, 100.0f, 4000.0f, 2.0f);

	// we could load multiple world here, but for this example we will stick with one
	// now set the loop running
	engine.startLoop();
}
