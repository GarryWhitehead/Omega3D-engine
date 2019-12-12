#include "Shadow.h"

#include "Core/Scene.h"

namespace OmegaEngine
{

Shadow::Shadow(Scene& scene)
    : scene(scene)
{
}

Shadow::~Shadow()
{
}

void Shadow::updateBuffer()
{
	lightPovDataSize = 0;

	Camera& camera = scene.getCurrCamera();

	for (auto& info : lights)
	{
		auto& light = std::get<0>(info);
		LightPOV* lightPovPtr = (LightPOV*)((uint64_t)lightPovData + (alignedPovDataSize * lightPovDataSize));

		OEMaths::mat4f projection =
		    OEMaths::perspective(light->fov, 1.0f, cameraManager.getZNear(), cameraManager.getZFar());
		OEMaths::mat4f view = OEMaths::lookAt(light->position, light->target, OEMaths::vec3f(0.0f, 1.0f, 0.0f));
		lightPovPtr->lightMvp = projection * view;

		light->lightMvp = lightPovPtr->lightMvp;

		++lightPovDataSize;
	}
}

}    // namespace OmegaEngine
