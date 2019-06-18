#pragma once

#include "Engine/engine.h"
#include "Models/GltfModel.h"
#include "Engine/world.h"
#include "Managers/CameraManager.h"
#include "ObjectInterface/Object.h"
#include "ObjectInterface/ComponentTypes.h"

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
		auto object = world->createGltfModelObject(model, true);
		object->addComponent<WorldTransformComponent>();

		// we can also use only certain attributes from the gltf model, and use other materials etc.
		//auto material = OmegaEngine::MaterialManager::createMaterial();

		//object->addComponent<MeshComponent>(model);
		//object->addComponent<MaterialComponent>(material);
	}

	// add a skybox
	world->addSkybox("skybox/cubemap.ktx", 0.5f);

	// and a default camera - multiple cameras can be added (TODO: switch via a designated key)
	world->addCameraToWorld();

	// we could load multiple world here, but for this example we will stick with one
	// now set the loop running
	engine.startLoop();

}