#include "LightManager.h"
#include "Objects/ObjectManager.h"
#include "Managers/ComponentInterface.h"

#include "tiny_gltf.h"

namespace OmegaEngine
{

	LightManager::LightManager()
	{
		VulkanAPI::MemoryAllocator &mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
		light_buffer = mem_alloc.allocate(VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(LightUboBuffer));
	}


	LightManager::~LightManager()
	{
	}

	void LightManager::parseGltfLight(uint32_t spaceId, tinygltf::Model& model)
	{
		
	}

	void LightManager::update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager, ComponentInterface* component_manager)
	{
		if (isDirty) {

			// TODO: update light positions, etc.

			// now update on the gpu side
			LightUboBuffer ubo;
			ubo.lightCount = lights.size();

			for (uint32_t i = 0; i < ubo.lightCount; ++i) {
				ubo.lights[i] = lights[i];
			}
			
			VulkanAPI::MemoryAllocator &mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
			mem_alloc.mapDataToSegment(light_buffer, &ubo, sizeof(LightUboBuffer));

			isDirty = false;
		}
	}
}
