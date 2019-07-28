#include "LightManager.h"
#include "Engine/Omega_Global.h"
#include "Managers/CameraManager.h"
#include "Managers/EventManager.h"
#include "OEMaths/OEMaths_transform.h"
#include "ObjectInterface/ComponentInterface.h"
#include "ObjectInterface/ObjectManager.h"
#include "VulkanAPI/BufferManager.h"
#include "tiny_gltf.h"

namespace OmegaEngine
{

LightManager::LightManager()
{
	// allocate the memory required for the light POV data
	alignedPovDataSize = VulkanAPI::Util::alignmentSize(sizeof(LightPOV));
	lightPovData = (LightPOV*)Util::alloc_align(alignedPovDataSize, alignedPovDataSize * MAX_LIGHTS);
}

LightManager::~LightManager()
{
	_aligned_free(lightPovData);
}

void LightManager::addLight(const LightType type, OEMaths::vec3f& position, OEMaths::vec3f& target,
                            OEMaths::vec3f& colour, float radius, float fov, float innerCone, float outerCone,
                            const LightAnimateType animType, const float animVel, const float animOffset)
{
	LightInfo light;
	light.position = position;
	light.target = target;
	light.colour = colour;
	light.type = type;
	light.radius = radius;
	light.fov = fov;
	light.innerCone = innerCone;
	light.outerCone = outerCone;

	// animation part
	LightAnimateInfo anim;
	anim.animationType = animType;
	anim.velocity = animVel;
	anim.offset = animOffset;

	lights.emplace_back(std::make_tuple(light, anim));

	isDirty = true;
}

void LightManager::addLight(const LightType type, OEMaths::vec3f& position, OEMaths::vec3f& target,
                            OEMaths::vec3f& colour, float radius, float fov, const LightAnimateType animType,
                            const float animVel, const float animOffset)
{
	LightInfo light;
	light.position = position;
	light.target = target;
	light.colour = colour;
	light.type = type;
	light.radius = radius;
	light.fov = fov;

	// animation part - defualt
	LightAnimateInfo anim;
	anim.animationType = animType;
	anim.velocity = animVel;
	anim.offset = animOffset;

	lights.emplace_back(std::make_tuple(light, anim));

	isDirty = true;
}

void LightManager::addLight(const LightInfo& light, const LightAnimateInfo& anim)
{
	lights.emplace_back(std::make_tuple(light, anim));
	// make sure this light is updated on the GPU side
	isDirty = true;
}

void LightManager::updateLightPositions(double time, double dt)
{
	// update the timer first - a pretty simple fudged timer but adequate for lighting
	// TODO: make this a config option
	constexpr float timerSpeed = 0.25f;

	timer += timerSpeed * (time / 1000000000);

	// clamp to 0.0f - 1.0f
	if (timer > 1.0)
	{
		timer -= 1.0f;
	}

	for (auto& info : lights)
	{
		auto light = std::get<0>(info);
		auto anim = std::get<1>(info);

		switch (anim.animationType)
		{
		case LightAnimateType::Static:
			break;
		case LightAnimateType::RotateX:
		{
			light.position.setY(anim.offset + std::abs(std::sin(OEMaths::radians(timer * 360.0f)) * anim.velocity));
			light.position.setZ(anim.offset + std::cos(OEMaths::radians(timer * 360.0f)) * anim.velocity);
			break;
		}
		case LightAnimateType::RotateY:
		{
			light.position.setX(anim.offset + std::abs(std::sin(OEMaths::radians(timer * 360.0f)) * anim.velocity));
			light.position.setZ(anim.offset + std::cos(OEMaths::radians(timer * 360.0f)) * anim.velocity);
			break;
		}
		case LightAnimateType::RotateZ:
		{
			light.position.setX(anim.offset + std::abs(std::sin(OEMaths::radians(timer * 360.0f)) * anim.velocity));
			light.position.setY(anim.offset + std::cos(OEMaths::radians(timer * 360.0f)) * anim.velocity);
			break;
		}
		}
		printf("position = x: %f, y: %f, z:%f \n", light.position.getX(), light.position.getY(), light.position.getZ());
		isDirty = true;
	}
}

void LightManager::updateDynamicBuffer(ComponentInterface* componentInterface)
{
	lightPovDataSize = 0;

	auto& cameraManager = componentInterface->getManager<CameraManager>();

	for (auto& info : lights)
	{
		auto light = std::get<0>(info);
		LightPOV* lightPovPtr = (LightPOV*)((uint64_t)lightPovData + (alignedPovDataSize * lightPovDataSize));

		OEMaths::mat4f projection =
		    OEMaths::perspective(light.fov, 1.0f, cameraManager.getZNear(), cameraManager.getZFar());
		OEMaths::mat4f view = OEMaths::lookAt(light.position, light.target, OEMaths::vec3f(0.0f, 1.0f, 0.0f));
		lightPovPtr->lightMvp = projection * view;

		light.lightMvp = lightPovPtr->lightMvp;

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
		lightBuffer.lightCount = static_cast<uint32_t>(lights.size());

		for (uint32_t i = 0; i < lightBuffer.lightCount; ++i)
		{
			lightBuffer.lights[i] = std::get<0>(lights[i]);
		}

		VulkanAPI::BufferUpdateEvent event{ "Light", (void*)&lightBuffer, sizeof(LightUboBuffer),
			                                VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };

		// let the buffer manager know that the buffers needs creating/updating via the event process
		Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);

		isDirty = false;
	}
}
}    // namespace OmegaEngine
