#pragma once

#include "Engine/engine.h"

// An example of building a scene using the component-object interface.
// Very much a work in progress at the moment.

int main(int argc, char *argv[])
{
	OmegaEngine::Engine engine("Scene Example", 1280, 700);

	// create a new empty world
	auto world = engine.createWorld("SceneOne");

	// Use a gltf image as one of our scene objects
	auto model = GltfModel::load("DamagedHelmet/DamagedHelmet.gltf");

	// create an object, using the model for vertices, materials, etc.
	auto& object = world->createNewObject();
	object->addComponent<GltfComponent>(worldMatrix);

	// we can also use only certain attributes from the gltf model, and use other materials etc.
	auto material = OmegaEngine::MaterialManager::createMaterial();

	object->addComponent<MeshComponent>(model);
	object->addComponent<MaterialComponent>(material);

	// add a skybox
	world->addSkybox("");

	// and a camera - multiple cameras can be added (TODO: switch via a designated key)
	auto &object world->createNewObject();
	object->addComponent<CameraComponent>();

	// we could load multiple world here, but for this example we will stick with one
	// now set the loop running
	engine.startLoop();

}