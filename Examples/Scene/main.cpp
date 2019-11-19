#pragma once

#include "Application.h"

#include "Core/Scene.h"
#include "Core/engine.h"
#include "Core/world.h"

#include "Rendering/Renderer.h"

#include "Types/NativeWindowWrapper.h"
#include "Types/Object.h"

#include "VulkanAPI/SwapChain.h"

#include "Models/Formats/GltfModel.h"

// An example of building a scene using the component-object interface.
// Very much a work in progress at the moment.

using namespace OmegaEngine;

int main(int argc, char* argv[])
{
	Application app;

	NativeWindowWrapper window;
	if (!app.init("Omega Engine Test", 1280, 800, window))
	{
		LOGGER_ERROR("Unable to initialise application. Exiting.....\n");
		exit(1);
	}

	Engine engine;
	if (!engine.init(window))
	{
		LOGGER_ERROR("Unable to initialise engine.\n");
		exit(1);
	}

	// create a new empty world
	World* world = engine.createWorld("SceneOne");

	// add a scene
	Scene* scene = world->createScene();

	// create a swapchain for rendering
	VulkanAPI::Swapchain swapchain = engine.createSwapchain(window);

	// Use a gltf image as one of our scene objects
	GltfModel model;
	if (!model.load("WaterBottle/WaterBottle.gltf"))
	{
		exit(1);
	}

	model.setWorldTrans({4.0f, 3.0f, 5.0});
	model.setWorldScale({15.0f});
	model.setWorldRotation({0.5, 0.0f, 0.5f, 0.5f});
	model.prepare();
	scene->addObject(model);

	// load a material json
	Material mat;
	mat.load("demo_rough.mat");

	// use this material with a stock primitive
	auto prim = Model::BuildStock();
	model.transform(-0.2f, 0.0f, 0.0f);
	model.type(Model::Stock::Plane);
	model.material(mat);
	scene->addObject(prim);

	// create the renderer - using a deffered renderer (only one supported at the moment)
	Renderer* renderer = engine.createRenderer(swapchain, scene);
	
	renderer->addSkybox("skybox/cubemap.ktx", 0.5f);

	// and a default camera - multiple cameras can be added (TODO: switch via a designated key)
	renderer->addCameraToWorld();

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
