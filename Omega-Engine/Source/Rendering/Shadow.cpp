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
