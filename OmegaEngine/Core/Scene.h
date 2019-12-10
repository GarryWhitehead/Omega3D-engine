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
class Renderable;

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

	Scene() = default;

	Scene(World& world, Engine& engine, VulkanAPI::VkDriver& driver);
	~Scene();

	// a scene isn't copyable
	Scene(Scene&) = delete;
	Scene& operator=(Scene&) = delete;

	void update();

	void prepare();

	void updateCameraBuffer();

	void updateTransformBuffer(const size_t staticModelCount, const size_t skinnedModelCount);

	Camera* getCurrentCamera();

	void getVisibleRenderables(Frustum& frustum, std::vector<VisibleCandidate>& renderables);

	friend class Renderer;

private:
	VulkanAPI::VkDriver& driver;

	/// per frame: all the renderables after visibility checks
	RenderQueue renderQueue;

	/// Current cameras associated with this scene.
	std::vector<Camera> cameras;
	uint16_t currCamera = UINT16_MAX;

	/// The world this scene is assocaited with
	World& world;
	Engine& engine;
};
}    // namespace OmegaEngine
