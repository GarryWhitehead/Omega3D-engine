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
			vertices_buffer = mem_alloc.allocate(VulkanAPI::MemoryUsage::VK_BUFFER_STATIC, vertices.size());
			indices_buffer = mem_alloc.allocate(VulkanAPI::MemoryUsage::VK_BUFFER_STATIC, indices.size());

			mem_alloc.mapDataToSegment(vertices_buffer, vertices.data(), vertices.size());
			mem_alloc.mapDataToSegment(indices_buffer, indices.data(), indices.size());
		}

		CubeModel::~CubeModel()
		{
		}

		PlaneModel::PlaneModel()
		{
			// vertices
			std::vector<Vertex> vertices = {
			{ { 1.0f, 1.0f },{ 1.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 0.0f }},
			{ { 0.0f, 1.0f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 0.0f }},
			{ { 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f }},
			{ { 1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f }}
			};

			// prepare indices
			std::vector<uint32_t> indices = { 0,1,2, 2,3,0 };
			for (uint32_t i = 0; i < 3; ++i)
			{
				uint32_t values[6] = { 0,1,2, 2,3,0 };
				for (auto index : values)
				{
					indices.push_back(i * 4 + index);
				}
			}

			VulkanAPI::MemoryAllocator& mem_alloc = VulkanAPI::Global::Managers::mem_allocator;

			// map vertices
			vertex_buffer = mem_alloc.allocate(VulkanAPI::MemoryUsage::VK_BUFFER_STATIC, sizeof(Vertex) * vertices.size());
			mem_alloc.mapDataToSegment(vertex_buffer, vertices.data(), vertices.size() * sizeof(Vertex));

			// map indices
			index_buffer = mem_alloc.allocate(VulkanAPI::MemoryUsage::VK_BUFFER_STATIC, sizeof(uint32_t) * indices.size());
			mem_alloc.mapDataToSegment(index_buffer, indices.data(), indices.size() * sizeof(uint32_t));
		}
	}
}