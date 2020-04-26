#include "ImageUtils/MappedTexture.h"
#include "ModelImporter/Formats/GltfModel.h"
#include "omega-engine/Application.h"
#include "omega-engine/Camera.h"
#include "omega-engine/Engine.h"
#include "omega-engine/LightManager.h"
#include "omega-engine/Object.h"
#include "omega-engine/RenderableManager.h"
#include "omega-engine/Skybox.h"
#include "omega-engine/IndirectLighting.h"
#include "utility/Logger.h"

#include <memory>

// An example of building a scene using the component-object interface.
// Very much a work in progress at the moment.

using namespace OmegaEngine;

int main(int argc, char* argv[])
{
    Application* app = Application::create("Omega Engine Test", 1280, 800);
    if (!app)
    {
        exit(1);
    }

    Engine* engine = app->createEngine(app->getWindow());
    if (!engine)
    {
        exit(1);
    }

    // create a new empty world
    World* world = engine->createWorld("SceneOne");

    // add a scene
    Scene* scene = world->createScene();
    scene->prepare();

    // create a swapchain for rendering
    SwapchainHandle swapchain = engine->createSwapchain(app->getWindow());

    // Use a gltf image as one of our scene objects
    GltfModel model;
    
    // The directory must be set to the location where all the model data is found - including
    // textures if any
    model.setDirectory(Util::String::append(Util::String(OE_ASSETS_DIR), "Models/WaterBottle/"));
    if (!model.load("WaterBottle.gltf"))
    {
        exit(1);
    }
    model.prepare();

    // we casn piece different meshes together so we have a parent object and their children
    // Or, we can have models that have no chilren, this is done via the createParentObj() as above
    for (auto& node : model.nodes)
    {
        // TODO: use a transform instance here to make this more user-friendly
        Object* modelObj = world->createParentObj({4.0f, 3.0f, 5.0}, {15.0f}, {0.5, 0.0f, 0.5f, 0.5f});

        RenderableInstance instance;
        MeshInstance* mesh = node->getMesh();
        instance.addMesh(mesh)
            .addNode(node.get())
            .addSkin(node->getSkin())
            .addMaterial(mesh->material)
            .create(*engine, modelObj);
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
    Renderer* renderer = engine->createRenderer(swapchain, scene);
    if (!renderer->prepare())
    {
        exit(1);
    }

    // load the skybox from disk
    auto envMap = std::make_unique<MappedTexture>();
    envMap->setDirectory(Util::String::append(Util::String(OE_ASSETS_DIR), "Models/skybox/"));
    envMap->load("cubemap.ktx");

    Skybox* skybox = world->createSkybox();
    skybox->setCubeMap(envMap.get()).setBlurFactor(0.5f);
    scene->addSkybox(skybox);
    
    // add indirect lighting to the scene
    IndirectLighting* ibl = world->createIndirectLighting();
    ibl->setEnvMap(skybox);
    scene->addIndirectLighting(ibl);
    
    // and a default camera
    Camera* camera = world->createCamera();
    // using dedfault parameters, need to still call "prepare()"
    camera->prepare();
    scene->addCamera(camera);

    // add a selection of different style lights to the scene
    LightManager::LightInstance sLight, pLight, dLight;
    Object* lightObj = world->createObj();
    sLight.setType(LightType::Spot).setPosition({3.0f, 1.0f, 1.0f}).create(*engine, lightObj);

    Object* lightObj2 = world->createObj();
    pLight.setType(LightType::Point).setPosition({0.0f, 2.0f, 3.0f}).create(*engine, lightObj2);

    Object* lightObj3 = world->createObj();
    dLight.setType(LightType::Directional)
        .setPosition({0.0f, 6.0f, 0.0f})
        .setIntensity(2000.0f)
        .create(*engine, lightObj3);

    // we could load multiple worlds here, but for this example we will stick with one
    // now set the loop running
    app->run(scene, renderer);
    
    engine->destroy();
}
