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

int main(int argc, char* argv[])
{
	Engine engine("Scene Example", 1280, 700);

	// create a new empty world
	auto world = engine.createWorld("SceneOne");

	{
		// Use a gltf image as one of our scene objects
		auto model = GltfModel::load("WaterBottle/WaterBottle.gltf");

		// create an object, using a gltf model for vertices, materials, etc.
		auto object = world->createGltfModelObject(model, OEMaths::vec3f{ 4.0f, 3.0f, 5.0f }, OEMaths::vec3f{ 15.0f },
		                                           OEMaths::quatf{ 0.5, 0.0f, 0.5f, 0.5f }, true);
	}

	{
		// Use a gltf image as one of our scene objects
		auto model = GltfModel::load("DamagedHelmet/DamagedHelmet.gltf");

		// create an object, using a gltf model for vertices, materials, etc.	
		auto object = world->createGltfModelObject(model, OEMaths::vec3f{ 0.0f, 3.0f, 0.0f }, OEMaths::vec3f{ 2.0f },
		                                           OEMaths::quatf{ 0.0f }, true);
	}

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
		auto object2 =
			world->createObject(OEMaths::vec3f{ -0.2f, 0.0f, 0.0f }, OEMaths::vec3f{ 1.0f }, OEMaths::quatf{ 0.0f });

		object2->addComponent<MeshComponent>(OEModels::generateQuadMesh(0.5f));
		object2->addComponent<MaterialComponent>("DemoRough", OEMaterials::Specular::Titanium, OEMaths::vec3f{ 0.3f },
												 OEMaths::vec4f{ 0.0f, 0.6f, 0.8f, 1.0f }, 0.8f, 0.2f);
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
