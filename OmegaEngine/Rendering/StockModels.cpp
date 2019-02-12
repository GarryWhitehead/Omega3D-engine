#include "StockModels.h"
#include "Vulkan/Vulkan_Global.h"

namespace OmegaEngine
{
	namespace RenderUtil
	{

		CubeModel::CubeModel()
		{
			// alloc mem buffer space for vertices and indices and upload to gpu
			VulkanAPI::MemoryAllocator& mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
			vertices_buffer = mem_alloc.allocate(VulkanAPI::MemoryUsage::VK_BUFFER_STATIC, vk::BufferUsageFlagBits::eVertexBuffer, vertices.size());
			indices_buffer = mem_alloc.allocate(VulkanAPI::MemoryUsage::VK_BUFFER_STATIC, vk::BufferUsageFlagBits::eIndexBuffer, indices.size());

			mem_alloc.mapDataToSegment(vertices_buffer, vertices.data(), vertices.size());
			mem_alloc.mapDataToSegment(indices_buffer, indices.data(), indices.size());
		}

		CubeModel::~CubeModel()
		{

		}
	}
}