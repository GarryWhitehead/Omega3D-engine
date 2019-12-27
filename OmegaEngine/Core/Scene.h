#pragma once

#include "Core/ObjectManager.h"

#include "Models/Formats/GltfModel.h"

#include "Rendering/RenderQueue.h"

#include "VulkanAPI/Buffer.h"

#include <vector>

namespace VulkanAPI
{
class VkDriver;
}

namespace OmegaEngine
{

// forward decleartions
class World;
class GltfModel;
class Object;
class Engine;
class Camera;
struct TransformInfo;
class Frustum;
struct Renderable;
class LightBase;
class Skybox;

class Scene
{
public:
	/**
	* @brief A temp struct used to gather viable renderable objects data ready for visibilty checks
	* and passing to the render queue
	*/
	struct VisibleCandidate
	{
		Renderable* renderable;
		TransformInfo* transform;
	};

	Scene(World& world, Engine& engine, VulkanAPI::VkDriver& driver);
	~Scene();

	// a scene isn't copyable
	Scene(Scene&) = delete;
	Scene& operator=(Scene&) = delete;

	void update();

	void prepare();

	void updateCameraBuffer();

	void updateTransformBuffer(std::vector<Scene::VisibleCandidate>& cand, const size_t staticModelCount, const size_t skinnedModelCount);

	void updateLightBuffer(std::vector<LightBase*> lights);

	Camera* getCurrentCamera();

	void getVisibleRenderables(Frustum& frustum, std::vector<VisibleCandidate>& renderables);

	void getVisibleLights(Frustum& frustum, std::vector<LightBase*>& renderables);

	friend class Renderer;

private:
	VulkanAPI::VkDriver& driver;

	/// per frame: all the renderables after visibility checks
	RenderQueue renderQueue;

	/// Current cameras associated with this scene.
	std::vector<Camera> cameras;
	uint16_t currCamera = UINT16_MAX;
    
    /// the skybox to be used with this scene. Also used for global illumination
    Skybox* skybox = nullptr;
    
	/// The world this scene is assocaited with
	World& world;
	Engine& engine;
};
}    // namespace OmegaEngine
