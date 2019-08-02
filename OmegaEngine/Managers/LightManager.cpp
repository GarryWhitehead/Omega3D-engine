#include "LightManager.h"
#include "Engine/Omega_Global.h"
#include "Managers/CameraManager.h"
#include "Managers/EventManager.h"
#include "OEMaths/OEMaths_transform.h"
#include "ObjectInterface/ComponentInterface.h"
#include "ObjectInterface/ObjectManager.h"
#include "VulkanAPI/BufferManager.h"


namespace OmegaEngine
{

LightManager::LightManager()
{
	// allocate the memory required for the light POV data
	alignedPovDataSize = VulkanAPI::Util::alignmentSize(sizeof(LightPOV));
	lightPovData = (LightPOV*)Util::alloc_align(alignedPovDataSize, alignedPovDataSize * (MAX_LIGHTS * 2));
}

LightManager::~LightManager()
{
	_aligned_free(lightPovData);
}

void LightManager::addSpotLight(const OEMaths::vec3f& position, const OEMaths::vec3f& target,
                                const OEMaths::vec3f& colour, const float fov, const OEMaths::vec3f dir, float radius,
                                float scale, float offset, const LightAnimateType animType, const float animVel)
{
	auto light = std::make_unique<SpotLight>();
	light->position = position;
	light->target = target;
	light->colour = colour;
	light->radius = radius;
	light->fov = fov;
	light->direction = dir;
	light->scale = scale;
	light->offset = offset;
	light->type = LightType::Spot;

	// animation part
	LightAnimateInfo anim;
	anim.animationType = animType;
	anim.velocity = animVel;

	lights.emplace_back(std::make_tuple(std::move(light), anim));

	isDirty = true;
}

void LightManager::addPointLight(const OEMaths::vec3f& position, const OEMaths::vec3f& target,
                                 const OEMaths::vec3f& colour, float fov, float radius, const LightAnimateType animType,
                                 const float animVel)
{
	auto light = std::make_unique<PointLight>();
	light->position = position;
	light->target = target;
	light->colour = colour;
	light->radius = radius;
	light->fov = fov;
	light->type = LightType::Point;

	// animation part - defualt
	LightAnimateInfo anim;
	anim.animationType = animType;
	anim.velocity = animVel;

	lights.emplace_back(std::make_tuple(std::move(light), anim));

	isDirty = true;
}

void LightManager::updateLightPositions(double time, double dt)
{
	// update the timer first - a pretty simple fudged timer but adequate for lighting
	// TODO: make this a config option
	constexpr float timerSpeed = 0.1f;

	timer += timerSpeed * (dt / 10000000);

	// clamp to 0.0f - 1.0f
	if (timer > 1.0)
	{
		timer -= 1.0f;
	}

	for (auto& info : lights)
	{
		auto& light = std::get<0>(info);
		auto& anim = std::get<1>(info);

		switch (anim.animationType)
		{
		case LightAnimateType::Static:
			break;
		case LightAnimateType::RotateX:
		{
			light->position.setY(std::abs(std::sin(OEMaths::radians(timer * 360.0f)) * anim.velocity));
			light->position.setZ(std::cos(OEMaths::radians(timer * 360.0f)) * anim.velocity);
			break;
		}
		case LightAnimateType::RotateY:
		{
			light->position.setX(std::abs(std::sin(OEMaths::radians(timer * 360.0f)) * anim.velocity));
			light->position.setZ(std::cos(OEMaths::radians(timer * 360.0f)) * anim.velocity);
			break;
		}
		case LightAnimateType::RotateZ:
		{
			light->position.setX(std::abs(std::sin(OEMaths::radians(timer * 360.0f)) * anim.velocity));
			light->position.setY(std::cos(OEMaths::radians(timer * 360.0f)) * anim.velocity);
			break;
		}
		}

		isDirty = true;
	}
}

void LightManager::updateDynamicBuffer(ComponentInterface* componentInterface)
{
	lightPovDataSize = 0;

	auto& cameraManager = componentInterface->getManager<CameraManager>();

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

	// now queue ready for uploading to the gpu
	VulkanAPI::BufferUpdateEvent event{ "LightDynamic", (void*)lightPovData, alignedPovDataSize * lightPovDataSize,
		                                VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };
	Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
}

void LightManager::updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager,
                               ComponentInterface* componentInterface)
{
	updateLightPositions(time, dt);

	if (isDirty)
	{
		// update dynamic buffers used by shadow pipeline
		updateDynamicBuffer(componentInterface);

		// now update ready for uploading on the gpu side
		uint32_t spotlightCount = 0;
		uint32_t pointlightCount = 0;

		for (auto& info : lights)
		{
			auto& light = std::get<0>(info);
			if (light->type == LightType::Spot)
			{
				const auto& spotLight = static_cast<SpotLight*>(light.get());

				// fill in the data to be sent to the gpu
				SpotLightUbo ubo(light->lightMvp,
								 OEMaths::vec4f{ spotLight->position, 1.0f },
				                 OEMaths::vec4f{ spotLight->direction, 1.0f }, spotLight->colour, spotLight->radius,
				                 spotLight->scale, spotLight->offset);
				lightBuffer.spotLights[spotlightCount++] = ubo;
			}
			else if (light->type == LightType::Point)
			{
				const auto& pointLight = static_cast<PointLight*>(light.get());

				// fill in the data to be sent to the gpu
				PointLightUbo ubo(light->lightMvp, OEMaths::vec4f{ pointLight->position, 1.0f },
				                 pointLight->colour, pointLight->radius);
				lightBuffer.pointLights[pointlightCount++] = ubo;
			}
		}

		lightBuffer.spotLightCount = spotlightCount;
		lightBuffer.pointLightCount = pointlightCount;

		VulkanAPI::BufferUpdateEvent event{ "Light", (void*)&lightBuffer, sizeof(LightUboBuffer),
			                                VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };

		// let the buffer manager know that the buffers needs creating/updating via the event process
		Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);

		isDirty = false;
	}
}
}    // namespace OmegaEngine
