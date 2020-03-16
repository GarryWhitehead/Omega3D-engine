#include "Shadow.h"

#include "Components/LightManager.h"
#include "Core/Camera.h"
#include "Core/engine.h"
#include "Core/Scene.h"
#include "OEMaths/OEMaths_transform.h"
#include "utility/AlignedAlloc.h"

namespace OmegaEngine
{

Shadow::Shadow(OEEngine& engine, OEScene& scene) : engine(engine), scene(scene)
{
}

Shadow::~Shadow()
{
}

void Shadow::updateBuffer()
{
    OECamera* camera = scene.getCurrentCamera();
    OELightManager* lightManager = engine.getLightManager();

    uint32_t count = 0;
    size_t lightCount = lightManager->lights.size();
    if (lightCount < 0)
    {
        return;
    }

    const size_t dynAlign = (sizeof(LightPOV) + 256 - 1) & ~(256 - 1);
    Util::AlignedAlloc aAlloc;
    aAlloc.alloc(dynAlign * lightCount, dynAlign);

    for (auto& light : lightManager->lights)
    {
        uint32_t offset = dynAlign * count++;
        LightPOV* lightPovPtr = (LightPOV*) ((uint64_t) aAlloc.getData() + offset);

        OEMaths::mat4f projection =
            OEMaths::perspective(light->fov, 1.0f, camera->getZNear(), camera->getZFar());
        OEMaths::vec3f up = {0.0f, 1.0f, 0.0f};
        OEMaths::mat4f view = OEMaths::lookAt(light->position, light->target, up);
        lightPovPtr->mvp = projection * view;

        light->lightMvp = lightPovPtr->mvp;
    }
}

} // namespace OmegaEngine
