#pragma once

#include "Components/TransformManager.h"
#include "Components/LightManager.h"
#include "Rendering/RenderQueue.h"
#include "Types/AABox.h"
#include "VulkanAPI/Buffer.h"
#include "omega-engine/Scene.h"
#include "omega-engine/Skybox.h"
#include "utility/CString.h"

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
class OEObject;
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
    static constexpr int MaxStaticModelCount = 100;
    static constexpr int MaxSkinnedModelCount = 100;
    static constexpr int MaxSpotlightCount = 20;
    static constexpr int MaxPointlightCount = 20;
    static constexpr int MaxDirlightCount = 2;

    // ubo buffer names used by the scene
    const Util::String cameraUboName = "CameraUbo";
    const Util::String staticTransUboName = "MeshUbo";
    const Util::String skinnedTransUboName = "SkinUbo";
    const Util::String lightUboName = "LightUbo";

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

    bool update(const double time);

    void prepare();

    void updateCameraBuffer();

    void updateTransformBuffer(
        std::vector<OEScene::VisibleCandidate>& cand,
        const size_t staticModelCount,
        const size_t skinnedModelCount);

    void updateLightBuffer(std::vector<LightBase*> lights);

    OECamera* getCurrentCamera();

    void getVisibleRenderables(Frustum& frustum, std::vector<VisibleCandidate>& renderables);

    void getVisibleLights(Frustum& frustum, std::vector<LightBase*>& renderables);

    VisibleCandidate buildRendCandidate(OEObject* obj, OEMaths::mat4f& worldMat);

    // ====== public functions for adding items to the scene ==============

    bool addSkybox(OESkybox* sb);

    void setCurrentCamera(OECamera* camera);

    friend class OERenderer;

private:
    // Ubo buffers for model/lighting data
    struct TransformUbo
    {
        OEMaths::mat4f modelMatrix;
    };

    struct SkinnedUbo
    {
        OEMaths::mat4f jointMatrices[TransformManager::MAX_BONE_COUNT];
        float jointCount;
    };

    // a mirror of the shader structs
    struct PointLightUbo
    {
        OEMaths::mat4f lightMvp;
        OEMaths::vec4f position;
        OEMaths::colour4 colour; //< rgb, intensity (lumens)
        float fallOut;
    };

    struct SpotLightUbo
    {
        OEMaths::mat4f lightMvp;
        OEMaths::vec4f position;
        OEMaths::vec4f direction;
        OEMaths::colour4 colour; //< rgb, intensity (lumens)
        float scale;
        float offset;
        float fallOut;
    };

    struct DirectionalLightUbo
    {
        OEMaths::mat4f lightMvp;
        OEMaths::vec4f position;
        OEMaths::vec4f direction;
        OEMaths::colour4 colour; //< rgb, intensity (lumens)
    };

    struct LightUbo
    {
        SpotLightUbo spotLights[MaxSpotlightCount];
        PointLightUbo pointLight[MaxPointlightCount];
        DirectionalLightUbo dirLight[MaxDirlightCount];
    };

private:

    VulkanAPI::VkDriver& driver;

    /// per frame: all the renderables after visibility checks
    RenderQueue renderQueue;

    /// Current camera used by this scene. The 'world' holds the ownership of the cma
    OECamera* camera;

    /// the skybox to be used with this scene. Also used for global illumination
    OESkybox* skybox;

    /// The world this scene is assocaited with
    OEWorld& world;
    OEEngine& engine;
};
} // namespace OmegaEngine
