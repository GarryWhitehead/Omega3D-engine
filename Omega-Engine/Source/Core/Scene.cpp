/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "Scene.h"

#include "Components/LightManager.h"
#include "Components/RenderableManager.h"
#include "Core/Camera.h"
#include "Core/Frustum.h"
#include "Core/ModelGraph.h"
#include "Core/World.h"
#include "Core/engine.h"
#include "ModelImporter/MeshInstance.h"
#include "Rendering/GBufferFillPass.h"
#include "Rendering/IndirectLighting.h"
#include "Threading/ThreadPool.h"
#include "Types/Skybox.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/AlignedAlloc.h"

namespace OmegaEngine
{

OEScene::OEScene(OEWorld& world, OEEngine& engine, VulkanAPI::VkDriver& driver)
    : driver(driver), world(world), engine(engine)
{
}

OEScene::~OEScene()
{
}

void OEScene::prepare()
{
    // camera ubo
    cameraUbo =
        driver.addUbo(cameraUboName, sizeof(OECamera::Ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    // model ubos
    size_t staticDataSize = MaxStaticModelCount * sizeof(TransformUbo);
    size_t skinnedDataSize = MaxSkinnedModelCount * sizeof(SkinnedUbo);

    meshUbo = driver.addUbo(staticTransUboName, staticDataSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    skinUbo =
        driver.addUbo(skinnedTransUboName, skinnedDataSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    // light ubo
    lightUbo = driver.addUbo(lightUboName, sizeof(LightUbo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
}

void OEScene::getVisibleRenderables(
    Frustum& frustum, std::vector<OEScene::VisibleCandidate>& renderables)
{
    if(!cullingEnabled)
    {
        for (auto& renderable : renderables)
        {
            renderable.renderable->visibility |= Renderable::Visible::Render;
        }
        return;
    }
    
    size_t workSize = renderables.size();

    auto visibilityCheck = [&frustum, &renderables](const size_t curr_idx, const size_t chunkSize) {
        assert(curr_idx + chunkSize <= renderables.size());
        for (size_t idx = curr_idx; idx < curr_idx + chunkSize; ++idx)
        {
           // TODO: this is here as a temporary measure - needs checking before this if culling is disabled so we don't bother wasting time checking
            if (frustum.checkBoxPlaneIntersect(renderables[idx].worldAABB))
            {
                renderables[idx].renderable->visibility |= Renderable::Visible::Render;
            }
        }
    };

    ThreadTaskSplitter splitWork {0, workSize, visibilityCheck};
    splitWork.run();
}

void OEScene::getVisibleLights(Frustum& frustum, std::vector<LightBase*>& lights)
{
    size_t workSize = lights.size();

    auto visibilityCheck = [&frustum, &lights](const size_t curr_idx, const size_t chunkSize) {
        assert(curr_idx + chunkSize <= lights.size());
        for (size_t idx = curr_idx; idx < curr_idx + chunkSize; ++idx)
        {
            LightBase* light = lights[idx];
            light->isVisible = false;

            if (light->type == LightType::Directional)
            {
                // no visisbility check on directional light
                light->isVisible = true;
                continue;
            }

            float radius;
            OEMaths::vec3f pos;
            if (light->type == LightType::Point)
            {
                PointLight* pLight = reinterpret_cast<PointLight*>(light);
                radius = pLight->radius;
                pos = pLight->position;
            }
            else if (light->type == LightType::Spot)
            {
                SpotLight* sLight = reinterpret_cast<SpotLight*>(light);
                radius = sLight->radius;
                pos = sLight->position;
            }
            else
            {
                LOGGER_WARN("Unrecognisied light type detected. This shouldn't happen!");
                continue;
            }

            // check whether this light is with the frustum boundaries
            if (frustum.checkSphereIntersect(pos, radius))
            {
                light->isVisible = true;
            }
        }
    };

    ThreadTaskSplitter splitWork {0, workSize, visibilityCheck};
    splitWork.run();
}

OEScene::VisibleCandidate OEScene::buildRendCandidate(OEObject* obj, OEMaths::mat4f& worldMat)
{
    auto& transManager = engine.getTransManager();
    auto* rendManager = engine.getRendManager();

    const ObjectHandle tHandle = transManager.getObjIndex(*obj);
    const ObjectHandle rHandle = rendManager->getObjIndex(*obj);

    VisibleCandidate candidate;
    candidate.renderable = &rendManager->getMesh(rHandle);
    candidate.transform = &transManager.getTransform(tHandle);

    // calculate the world-orientated AABB
    OEMaths::mat4f localMat = candidate.transform->modelTransform;
    candidate.worldTransform = worldMat * localMat;

    AABBox box {candidate.renderable->instance->dimensions.min,
                candidate.renderable->instance->dimensions.max};
    candidate.worldAABB = AABBox::calculateRigidTransform(box, candidate.worldTransform);
    return candidate;
}

bool OEScene::update(const double time)
{
    // update the managers first
    engine.getAnimManager().update(time, engine);
    engine.getLightManager()->update(*camera);
    
    if (!engine.getRendManager()->update())
    {
        return false;
    }

    auto& objects = world.getObjectsList();
    auto& models = world.getModelGraph().getNodeList();

    // clear the render queue
    renderQueue.resetAll();

    // we create a temp container as we will be doing the visibility checks async
    // reserve more space than we need
    std::vector<VisibleCandidate> candRenderableObjs;
    candRenderableObjs.reserve(objects.size());

    // iterate through the model graph add as a possible candidate if active
    for (ModelGraph::Node* node : models)
    {
        // if the parent is inactive, then all its children are too
        if (!node->parent->isActive())
        {
            continue;
        }

        OEMaths::mat4f worldMat = node->world.worldMat;

        // the parent
        VisibleCandidate candidate = buildRendCandidate(node->parent, worldMat);
        candRenderableObjs.emplace_back(candidate);

        // and the children if any
        for (OEObject* child : node->children)
        {
            if (!child->isActive())
            {
                continue;
            }

            VisibleCandidate childCand = buildRendCandidate(child, worldMat);
            candRenderableObjs.emplace_back(childCand);
        }
    }

    // now for the lights. At the moment we iterate through the list of objects and find any that
    // have a light component. If they are active then these are added as a potential candiate
    // lighting source
    auto* lightManager = engine.getLightManager();

    std::vector<LightBase*> candLightObjs;
    candLightObjs.reserve(objects.size());

    for (OEObject* obj : objects)
    {
        if (!obj->isActive())
        {
            continue;
        }

        ObjectHandle lHandle = lightManager->getObjIndex(*obj);
        if (lHandle.valid())
        {
            candLightObjs.emplace_back(lightManager->getLight(lHandle));
        }
    }

    // prepare the camera frustum
    // update the camera matrices before constructing the fustrum
    Frustum frustum;
    camera->updateViewMatrix();
    frustum.projection(camera->getProjMatrix() * camera->getViewMatrix());

    // ============ visibility checks and culling ===================
    // first renderables - split work tasks and run async - Sets the visibility bit if passes
    // intersection test This will then be used to generate the render queue
    getVisibleRenderables(frustum, candRenderableObjs);
    
    // shadow culling tests
    // ** TODO **

    // and prepare the visible lighting list
    getVisibleLights(frustum, candLightObjs);

    // ============ render queue generation =========================
    std::vector<RenderableQueueInfo> queueRend;

    // key a count of the number of static and skinned models for later
    size_t staticModelCount = 0;
    size_t skinnedModelCount = 0;

    for (const VisibleCandidate& cand : candRenderableObjs)
    {
        Renderable* rend = cand.renderable;
        // only add visible renderables to the queue
        if (!rend->visibility.testBit(Renderable::Visible::Render))
        {
            continue;
        }

        if (rend->instance->variantBits.testBit(MeshInstance::Variant::HasSkin))
        {
            ++skinnedModelCount;
        }
        ++staticModelCount;

        RenderableQueueInfo queueInfo;
        // we use the renderable data as it is, rather than waste time copying everything into
        // another struct. This method does mean that it is imperative that the data isnt destroyed
        // until the beginning of the next frame and that the data isn't written too - we aren't
        // using guards though this might be required.
        queueInfo.renderableData = (void*) rend;
        queueInfo.renderableHandle = this;
        queueInfo.renderFunction = GBufferFillPass::drawCallback;
        queueInfo.sortingKey = RenderQueue::createSortKey(
            RenderQueue::Layer::Default, rend->materialId, rend->instance->variantBits.getUint64());
        queueRend.emplace_back(queueInfo);
    }
    renderQueue.pushRenderables(queueRend, RenderQueue::Type::Colour);

    // ================== update ubos =================================
    // camera buffer is updated every frame as we expect this to change a lot
    updateCameraBuffer();

    // we also update the transforms every frame though could have a dirty flag
    updateTransformBuffer(candRenderableObjs, staticModelCount, skinnedModelCount);

    updateLightBuffer(candLightObjs);

    return true;
}

void OEScene::updateCameraBuffer()
{
    // update everything in the buffer
    OECamera::Ubo ubo;
    ubo.mvp = camera->getMvpMatrix();
    ubo.cameraPosition = camera->getPos();
    ubo.projection = camera->getProjMatrix();
    ubo.model = camera->getModelMatrix(); // this is just identity for now
    ubo.view = camera->getViewMatrix();
    ubo.zNear = camera->getZNear();
    ubo.zFar = camera->getZFar();

    cameraUbo->map(&ubo, sizeof(OECamera::Ubo));
}

void OEScene::updateTransformBuffer(
    std::vector<OEScene::VisibleCandidate>& candObjects,
    const size_t staticModelCount,
    const size_t skinnedModelCount)
{
    // Dynamic buffers are aligned to >256 bytes as designated by the Vulkan spec
    const size_t staticDynAlign = (sizeof(TransformUbo) + 256 - 1) & ~(256 - 1);
    const size_t skinDynAlign = (sizeof(SkinnedUbo) + 256 - 1) & ~(256 - 1);
    Util::AlignedAlloc skinAlignAlloc;
    Util::AlignedAlloc staticAlignAlloc;

    if (staticModelCount > 0)
    {
        staticAlignAlloc.alloc(staticDynAlign * staticModelCount, staticDynAlign);
        assert(!staticAlignAlloc.empty());
    }
    if (skinnedModelCount > 0)
    {
        skinAlignAlloc.alloc(skinDynAlign * skinnedModelCount, skinDynAlign);
        assert(!skinAlignAlloc.empty());
    }

    size_t staticCount = 0;
    size_t skinnedCount = 0;

    for (auto& cand : candObjects)
    {
        Renderable* rend = cand.renderable;
        if (!rend->visibility.testBit(Renderable::Visible::Render))
        {
            continue;
        }

        TransformInfo* transInfo = cand.transform;

        size_t meshOffset = staticDynAlign * staticCount++;
        TransformUbo* currStaticPtr =
            (TransformUbo*) ((uint64_t) staticAlignAlloc.getData() + (meshOffset));
        currStaticPtr->modelMatrix = transInfo->modelTransform;

        // the dynamic buffer offsets are stored in the renderable for ease of access when
        // drawing
        rend->meshDynamicOffset = meshOffset;

        if (!transInfo->jointMatrices.empty())
        {
            size_t skinOffset = skinDynAlign * skinnedCount++;
            rend->skinDynamicOffset = skinOffset;

            SkinnedUbo* currSkinnedPtr =
                (SkinnedUbo*) ((uint64_t) skinAlignAlloc.getData() + (skinDynAlign * skinnedCount++));

            // rather than throw an error, clamp the joint if it exceeds the max
            uint32_t jointCount = std::min(
                TransformManager::MAX_BONE_COUNT,
                static_cast<uint32_t>(transInfo->jointMatrices.size()));
            memcpy(
                currSkinnedPtr->jointMatrices,
                transInfo->jointMatrices.data(),
                jointCount * sizeof(OEMaths::mat4f));
            rend->skinDynamicOffset = skinOffset;
        }
    }

    // Static and skinned model buffers have three outcomes - new instances are created if this is
    // the first frame, the new data is mapped to the existing buffer if the data sisze is within
    // the current buffer size or the buffer is destroyed and a new static buffer created
    if (staticModelCount > 0)
    {
        size_t staticDataSize = staticModelCount * sizeof(TransformUbo);
        meshUbo->map(staticAlignAlloc.getData(), staticDataSize);
    }

    // skinned buffer
    if (skinnedModelCount > 0)
    {
        size_t skinnedDataSize = skinnedModelCount * sizeof(SkinnedUbo);
        skinUbo->map(skinAlignAlloc.getData(), skinnedDataSize);
    }
}

void OEScene::updateLightBuffer(std::vector<LightBase*> candLights)
{
    uint32_t spotlightCount = 0;
    uint32_t pointlightCount = 0;
    uint32_t dirLightCount = 0;

    LightUbo ubo;

    // copy the light attributes we need for use in the light shaders.
    for (LightBase* light : candLights)
    {
        if (!light->isVisible)
        {
            continue;
        }

        if (light->type == LightType::Spot)
        {
            const auto& spotLight = static_cast<SpotLight*>(light);

            // fill in the data to be sent to the gpu
            SpotLightUbo spotlightUbo {light->lightMvp,
                                       OEMaths::vec4f {spotLight->position, 1.0f},
                                       OEMaths::vec4f {spotLight->target, 1.0f},
                                       {spotLight->colour, spotLight->intensity},
                                       spotLight->scale,
                                       spotLight->offset,
                                       spotLight->fallout};
            ubo.spotLights[spotlightCount++] = spotlightUbo;
        }
        else if (light->type == LightType::Point)
        {
            const auto& pointLight = static_cast<PointLight*>(light);

            // fill in the data to be sent to the gpu
            PointLightUbo pointlightUbo {light->lightMvp,
                                         OEMaths::vec4f {pointLight->position, 1.0f},
                                         {pointLight->colour, pointLight->intensity},
                                         pointLight->fallOut};
            ubo.pointLight[pointlightCount++] = pointlightUbo;
        }
        else if (light->type == LightType::Directional)
        {
            const auto& dirLight = static_cast<DirectionalLight*>(light);

            // fill in the data to be sent to the gpu
            DirectionalLightUbo dirlightUbo {dirLight->lightMvp,
                                             OEMaths::vec4f {dirLight->position, 1.0f},
                                             OEMaths::vec4f {dirLight->target, 1.0f},
                                             {dirLight->colour, dirLight->intensity}};
            ubo.dirLight[dirLightCount++] = dirlightUbo;
        }
    }

    lightUbo->map(&ubo, sizeof(LightUbo));
}

void OEScene::setCurrentCamera(OECamera* cam)
{
    assert(cam);
    camera = cam;
}

OECamera* OEScene::getCurrentCamera()
{
    return camera;
}

bool OEScene::addSkybox(OESkybox* sb)
{
    assert(sb);
    if (!sb->cubeMap->isCubeMap())
    {
        LOGGER_ERROR(
            "Trying to add a skybox which has the incorrect texture type - must be a cubemap.");
        return false;
    }
    skybox = sb;

    return true;
}

void OEScene::addIndirectLighting(OEIndirectLighting* il)
{
    assert(il);
    ibl = il;
}

OESkybox* OEScene::getSkybox()
{
    return skybox;
}

// ============================ front-end =========================================

void Scene::addCamera(Camera* camera)
{
    static_cast<OEScene*>(this)->setCurrentCamera(static_cast<OECamera*>(camera));
}

void Scene::prepare()
{
    static_cast<OEScene*>(this)->prepare();
}

Camera* Scene::getCurrentCamera()
{
    return static_cast<OEScene*>(this)->getCurrentCamera();
}

bool Scene::addSkybox(Skybox* instance)
{
    return static_cast<OEScene*>(this)->addSkybox(static_cast<OESkybox*>(instance));
}

void Scene::addIndirectLighting(IndirectLighting* ibl)
{
    static_cast<OEScene*>(this)->addIndirectLighting(static_cast<OEIndirectLighting*>(ibl));
}

} // namespace OmegaEngine
