#pragma once

#include "omega-engine/Scene.h"
#include "omega-engine/Skybox.h"

#include "Core/ObjectManager.h"
#include "Core/ModelGraph.h"

#include "Types/AABox.h"

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
class OEWorld;
class GltfModel;
class Object;
class OEEngine;
class OECamera;
struct TransformInfo;
class Frustum;
struct Renderable;
class LightBase;
class OESkybox;
class SkyboxInstance;

class OEScene : public Scene
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
        AABBox worldAABB;
        OEMaths::mat4f worldTransform;
	};

	OEScene(OEWorld& world, OEEngine& engine, VulkanAPI::VkDriver& driver);
	~OEScene();

	// a OEScene isn't copyable
	OEScene(OEScene&) = delete;
	OEScene& operator=(OEScene&) = delete;

	void update();

	void prepare();

	void updateCameraBuffer();

	void updateTransformBuffer(std::vector<OEScene::VisibleCandidate>& cand, const size_t staticModelCount, const size_t skinnedModelCount);

	void updateLightBuffer(std::vector<LightBase*> lights);

	OECamera* getCurrentCamera();

	void getVisibleRenderables(Frustum& frustum, std::vector<VisibleCandidate>& renderables);

	void getVisibleLights(Frustum& frustum, std::vector<LightBase*>& renderables);
    
    VisibleCandidate buildRendCandidate(Object* obj, OEMaths::mat4f& worldMat);
    
    // ====== public functions for adding items to the scene ==============
    
    bool addSkybox(Skybox::Instance& instance);
    
    void addCamera(const OECamera camera);
    
	friend class Renderer;

private:
	VulkanAPI::VkDriver& driver;

	/// per frame: all the renderables after visibility checks
	RenderQueue renderQueue;
    
    /// the model graph for renderable objects - links parent objects with their children.
    /// will probably become more of a scene graph at some point.
    ModelGraph modelGraph;
    
	/// Current cameras associated with this scene.
	std::vector<OECamera> cameras;
	uint16_t currCamera = UINT16_MAX;
    
    /// the skybox to be used with this scene. Also used for global illumination
    std::unique_ptr<OESkybox> skybox;
    
	/// The world this scene is assocaited with
	OEWorld& world;
	OEEngine& engine;
};
}    // namespace OmegaEngine
