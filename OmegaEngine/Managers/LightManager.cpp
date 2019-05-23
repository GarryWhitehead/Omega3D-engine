#include "LightManager.h"
#include "Objects/ObjectManager.h"
#include "Managers/ComponentInterface.h"
#include "Vulkan/BufferManager.h"
#include "Managers/EventManager.h"
#include "Engine/Omega_Global.h"
#include "tiny_gltf.h"

namespace OmegaEngine
{

	LightManager::LightManager()
	{
	}


	LightManager::~LightManager()
	{
	}

	void LightManager::parseGltfLight(uint32_t spaceId, tinygltf::Model& model)
	{
		
	}

	void LightManager::updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager, ComponentInterface* componentInterface)
	{
		if (isDirty) 
		{
			// TODO: update light positions, etc.

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
