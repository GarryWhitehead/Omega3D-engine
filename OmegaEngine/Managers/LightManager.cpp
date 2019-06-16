#include "LightManager.h"
#include "ObjectInterface/ObjectManager.h"
#include "ObjectInterface/ComponentInterface.h"
#include "Managers/CameraManager.h"
#include "VulkanAPI/BufferManager.h"
#include "Managers/EventManager.h"
#include "Engine/Omega_Global.h"
#include "OEMaths/OEMaths_transform.h"
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

	void LightManager::parseGltfLight(uint32_t spaceId, tinygltf::Model& model)
	{

	}

	void LightManager::updateDynamicBuffer(ComponentInterface* componentInterface)
	{
		lightPovDataSize = 0;

		auto& cameraManager = componentInterface->getManager<CameraManager>();

		for (auto& light : lights)
		{
			LightPOV* lightPovPtr = (LightPOV*)((uint64_t)lightPovData + (alignedPovDataSize * lightPovDataSize));

			OEMaths::mat4f projection = OEMaths::perspective(light.fov, 1.0f, cameraManager.getZNear(), cameraManager.getZFar());
			OEMaths::mat4f view = OEMaths::lookAt(light.position, light.target, OEMaths::vec3f(0.0f, 1.0f, 0.0f));
			lightPovPtr->lightMvp = projection * view;

			++lightPovDataSize;
		}

		// now queue ready for uploading to the gpu
		VulkanAPI::BufferUpdateEvent event{ "LightDynamic", (void*)lightPovData, alignedPovDataSize * lightPovDataSize, VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };
		Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
	}

	void LightManager::updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager, ComponentInterface* componentInterface)
	{
		if (isDirty) 
		{
			// TODO: update light positions, etc.

			// update dynamic buffers used by shadow pipeline
			updateDynamicBuffer(componentInterface);

			// now update ready for uploading on the gpu side
			lightBuffer.lightCount = static_cast<uint32_t>(lights.size());

			for (uint32_t i = 0; i < lightBuffer.lightCount; ++i) 
			{
				lightBuffer.lights[i] = lights[i];
			}

			VulkanAPI::BufferUpdateEvent event{ "Light", (void*)&lightBuffer, sizeof(LightUboBuffer), VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };

			// let the buffer manager know that the buffers needs creating/updating via the event process
			Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
				
			isDirty = false;
		}
	}
}
