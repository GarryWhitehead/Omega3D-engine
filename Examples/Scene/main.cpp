#include "omega-engine/Application.h"
#include "omega-engine/Engine.h"
#include "omega-engine/Skybox.h"
#include "omega-engine/LightManager.h"
#include "omega-engine/RenderableManager.h"
#include "omega-engine/ObjectManager.h"
#include "omega-engine/Camera.h"

#include "utility/Logger.h"

#include "ModelImporter/Formats/GltfModel.h"

#include "ImageUtils/MappedTexture.h"

#include <memory>

// An example of building a scene using the component-object interface.
// Very much a work in progress at the moment.

using namespace OmegaEngine;

int main(int argc, char* argv[])
{
	Application app;

	WindowInstance window;
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
    SwapchainHandle swapchain = engine.createSwapchain(window);

	// Use a gltf image as one of our scene objects
	GltfModel model;
	if (!model.load("WaterBottle/WaterBottle.gltf"))
	{
		exit(1);
	}
    
    // we require the object manager for creating new object instances
    auto& objManager = world->getObjManager();
    
    // we can adjust the model transformation.
    model.setWorldTrans({4.0f, 3.0f, 5.0}).setWorldScale({15.0f}).setWorldRotation({0.5, 0.0f, 0.5f, 0.5f}).prepare();
    
    // add each node of the model - this will 'always' be one or more
    for (auto& node : model.getNodes())
    {
        Object* modelObj = objManager.createObject();
        RenderableManager::Instance instance;
        MeshInstance* mesh = node.getMesh();
    instance.addMesh(mesh).addNode(&node).addSkin(node.getSkin()).addMaterial(mesh->getMaterial()).create(engine, modelObj);
    }
    
    /*
    // create a stock primitive and apply our own material from file
	// load a material json - this is slow and binary format should be preferred
	Material mat;
	mat.load("demo_rough.mat");

	// use this material with a stock primitive
	auto prim = Model::BuildStock();
	model.transform(-0.2f, 0.0f, 0.0f);
	model.type(Model::Stock::Plane);
	model.material(mat);
	scene->addObject(prim);*/

	// create the renderer - using a deffered renderer (only one supported at the moment)
	Renderer* renderer = engine.createRenderer(window, swapchain, scene);
	
    // load the skybox from disk
    auto envMap = std::make_unique<MappedTexture>();
    envMap->load("skybox/cubemap.ktx");
    
    Skybox::Instance skybox;
    skybox.setCubeMap(envMap.get()).setBlurFactor(0.5f);
	scene->addSkybox(skybox);

	// and a default camera - multiple cameras can be added (TODO: switch via a designated key)
    Camera camera;
    scene->addCamera(camera);

	// add a selection of different style lights to the scene
    LightManager::LightInstance sLight, pLight, dLight;
    sLight.setType(LightType::Spot).setPosition({3.0f, 1.0f, 1.0f}).create(engine);
    
    pLight.setType(LightType::Point).setPosition({0.0f, 2.0f, 3.0f}).create(engine);
    
    dLight.setType(LightType::Directional).setPosition({0.0f, 6.0f, 0.0f}).setIntensity(2000.0f).create(engine);

	// we could load multiple world here, but for this example we will stick with one
	// now set the loop running
    app.run(*scene, *renderer);
}
