#include "Scene.h"

#include "Types/Object.h"

#include "Core/Camera.h"
#include "Core/World.h"
#include "Core/engine.h"

#include "Components/LightManager.h"
#include "Components/RenderableManager.h"
#include "Components/TransformManager.h"

#include "VulkanAPI/VkDriver.h"

namespace OmegaEngine
{

Scene::Scene(World& world, Engine& engine)
    : world(world)
    , engine(engine)
{
}

Scene::~Scene()
{
}

void Scene::prepare()
{
	// prepare the camera buffer - note: the id matches the naming of the shader ubo
	// the data will be updated on a per frame basis in the update
	auto& driver = engine.getVkDriver();
	driver.addUbo("CameraUbo", sizeof(Camera::Ubo), Buffer::Usage::Dynamic);
}

void Scene::update()
{
	auto& transManager = engine.getTransManager();
	auto& rendManager = engine.getRendManager();
	auto& lightManager = engine.getLightManager();

	auto& objects = world.getObjManager().getObjectsList();

	// reserve more space than we need
	std::vector<VisibleCandidate> candObjects(objects.size());

	// create a temp list of all renderable and light objects that are active
	// we create a temp container as we will be doing the visibility checks async
	for (Object& obj : objects)
	{
		if (!obj.isActive())
		{
			continue;
		}

		ObjHandle rHandle = rendManager.getObjIndex(obj);
		ObjHandle tHandle = transManager.getObjIndex(obj);

		// it should be impossible for a object to exsist that doesn't have both a transform and renderable component, but better make sure!
		if (rHandle && tHandle)
		{
			VisibleCandidate candidate;
			candidate.renderable = &rendManager.getMesh(rHandle);
			candidate.transform = &transManager.getTransform(tHandle);
			candObjects.emplace_back(candidate);
		}
		else
		{
			ObjHandle lHandle = lightManager.getObjIndex(obj);
			if (lHandle)
			{
				// TODO
			}
		}
	}

	// ============ visibility checks and culling ===================
	// first renderables - seperate work and run async - fills the render queue
	// which will be displayed by the renderer


	// and prepare the visible lighting list
}

void Scene::updateCamera()
{
	Camera& camera = cameras[currCamera];

	camera.updateViewMatrix();

	// update everything in the buffer
	Camera::Ubo ubo;
	ubo.mvp = camera.getMvpMatrix();
	ubo.cameraPosition = camera.getPos();
	ubo.projection = camera.getProjMatrix();
	ubo.model = camera.getModelMatrix();    // this is just identity for now
	ubo.view = camera.getViewMatrix();
	ubo.zNear = camera.getZNear();
	ubo.zFar = camera.getZFar();

	auto& driver = engine.getVkDriver();
	driver.updateUniform("CameraUbo", &ubo, sizeof(Camera::Ubo));
}

Camera* Scene::getCurrentCamera()
{
	return &cameras[currCamera];
}

}    // namespace OmegaEngine
