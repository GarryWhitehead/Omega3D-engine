#pragma once

#include "Application.h"

#include "Core/Scene.h"
#include "Core/engine.h"
#include "Core/world.h"
#include "Managers/CameraManager.h"
#include "Managers/LightManager.h"
#include "Managers/TransformManager.h"
#include "Models/Gltf/GltfModel.h"
#include "Models/OEMaterials.h"
#include "Models/OEModels.h"
#include "Types/ComponentTypes.h"
#include "Types/Object.h"

// An example of building a scene using the component-object interface.
// Very much a work in progress at the moment.

using namespace OmegaEngine;

int main(int argc, char* argv[])
{
	Application app;

	Engine engine;

	// create a new empty world
	World* world = engine.createWorld("SceneOne");

	// add a scene
	Scene* scene = world->createScene();

	// Use a gltf image as one of our scene objects
	auto model = GltfModel::create("WaterBottle/WaterBottle.gltf");
	model.worldTransform(4.0f, 3.0f, 5.0);
	model.worldScale(15.0f);
	model.worldRotation(0.5, 0.0f, 0.5f, 0.5f);
	model.build(world, obj);
	scene.addObject(obj);

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
		Object obj =
		    ObjectManager::create(OEMaths::vec3f{ -0.2f, 0.0f, 0.0f }, OEMaths::vec3f{ 1.0f }, OEMaths::quatf{ 0.0f });

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
	world->addSpotLightToWorld({ 0.0f, 3.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f }, 100.0f, 1000.0f, 10.0f, 0.5f,
	                           0.5f);
	world->addDirectionalLightToWorld({ 0.0f, 0.0f, 0.0f }, { 8.0f, 8.0f, 0.0f }, { 0.9f, 0.8f, 0.2f }, 100.0f,
	                                  10000.0f);
	world->addPointLightToWorld({ 3.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f }, 100.0f, 4000.0f, 2.0f);

	// we could load multiple world here, but for this example we will stick with one
	// now set the loop running
	engine.startLoop();
}
