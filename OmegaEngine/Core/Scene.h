#pragma once

#include "Core/ObjectManager.h"

#include "Models/Formats/GltfModel.h"

#include "Rendering/RenderQueue.h"

#include <vector>

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

	Scene(World& world, Engine& engine);
	~Scene();

	// a scene isn't copyable
	Scene(Scene&) = delete;
	Scene& operator=(Scene&) = delete;

	void update();

	void prepare();

	void updateCamera();

	Camera* getCurrentCamera();

	void getVisibleRenderables(Frustum& frustum, std::vector<VisibleCandidate>& renderables);

	friend class Renderer;

private:

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
