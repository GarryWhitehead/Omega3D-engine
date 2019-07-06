#pragma once

#include "Engine/engine.h"
#include "Engine/world.h"
#include "Managers/CameraManager.h"
#include "Managers/LightManager.h"
#include "Models/GltfModel.h"
#include "ObjectInterface/ComponentTypes.h"
#include "ObjectInterface/Object.h"
#include "Models/OEModels.h"
#include "Models/OEMaterials.h"

// An example of building a scene using the component-object interface.
// Very much a work in progress at the moment.

using namespace OmegaEngine;

int main(int argc, char *argv[])
{
	Engine engine("Scene Example", 1280, 700);

	// create a new empty world
	auto world = engine.createWorld("SceneOne");

	{
		// Use a gltf image as one of our scene objects
		auto model = GltfModel::load("DamagedHelmet/DamagedHelmet.gltf");

		// create an object, using a gltf model for vertices, materials, etc.
		auto object = world->createGltfModelObject(model, OEMaths::vec3f{0.0f, 0.5f, 0.1f}, OEMaths::vec3f{1.0f}, OEMaths::quatf{0.0f}, true);
	}

	// adding stock models to the scene
	auto object = world->createObject(OEMaths::vec3f{ 0.2f, 0.0f, 0.0f }, OEMaths::vec3f{ 1.0f },
	                                  OEMaths::quatf{ 0.0f });
	object->addComponent<OEModelComponent>(Models::OEModels::generateSphere(30));
	object->addComponent<MaterialComponent>("DemoMaterial", OEMaterials::Specular::Gold, OEMaths::vec3f{0.3f}, OEMaths::vec4f{0.5f, 0.2f, 0.0f, 1.0f}, 0.2f, 1.0f);

	// add a skybox
	world->addSkybox("skybox/cubemap.ktx", 0.5f);

	// and a default camera - multiple cameras can be added (TODO: switch via a designated key)
	world->addCameraToWorld();

	// add different lights
	world->addLightToWorld(LightType::Spot, { 0.0f, 3.0f, 10.0f }, { 0.0f, 0.0f, 0.0f },
	                       { 1.0f}, 50.0f, 100.0f);
	//world->addLightToWorld(LightType::Spot, { 0.0f, -1.0f, 0.0f }, { -2.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 150.0f, 100.0f);
	//world->addLightToWorld(LightType::Cone, { 0.0f, -1.0f, -5.0f }, { 2.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }, 50.0f, 80.0f, 15.0f, 25.0f);

	// we could load multiple world here, but for this example we will stick with one
	// now set the loop running
	engine.startLoop();
}
