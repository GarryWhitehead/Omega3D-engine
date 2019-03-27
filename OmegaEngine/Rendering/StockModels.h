#pragma once

#include "Vulkan/Common.h"
#include "Vulkan/MemoryAllocator.h"
#include "OEMaths/OEMaths.h"

#include <cstdint>
#include <array>

namespace OmegaEngine
{
	namespace RenderUtil
	{
		class CubeModel
		{
		public:

			// cube vertices
			std::array<float, 24> vertices
			{
				// front
				-1.0f, -1.0f, 1.0f,
				1.0f, -1.0f, 1.0f,
				1.0f, 1.0f, 1.0f,
				-1.0f, 1.0f, 1.0f,
				// back 
				-1.0f, -1.0f, -1.0f,
				1.0f, -1.0f, -1.0f,
				1.0f, 1.0f, -1.0f,
				-1.0f, 1.0f, -1.0f
			};

			// cube indices
			std::array<uint32_t, 36> indices
			{
				// front
				0, 1, 2,
				2, 3, 0,
				// right side
				1, 5, 6,
				6, 2, 1,
				// back
				7, 6, 5,
				5, 4, 7,
				// left side
				4, 0, 3,
				3, 7, 4,
				// bottom
				4, 5, 1,
				1, 0, 4,
				// top
				3, 2, 6,
				6, 7, 3
			};

			CubeModel();
			~CubeModel();

			VulkanAPI::MemorySegment& get_vertex_buffer()
			{
				return vertices_buffer;
			}

			VulkanAPI::MemorySegment& get_index_buffer()
			{
				return indices_buffer;
			}

			uint32_t get_index_count() const
			{
				return static_cast<uint32_t>(indices.size());
			}

		private:

			VulkanAPI::MemorySegment vertices_buffer;
			VulkanAPI::MemorySegment indices_buffer;

		};

		class PlaneModel
		{
		public:
			
			struct Vertex
			{
				OEMaths::vec2f uv;
				OEMaths::vec3f pos;
				OEMaths::vec3f normal;
			};

			VulkanAPI::MemorySegment& get_vertex_buffer()
			{
				return vertex_buffer;
			}

			VulkanAPI::MemorySegment& get_index_buffer()
			{
				return index_buffer;
			}

			PlaneModel();

		private:

			VulkanAPI::MemorySegment vertex_buffer;
			VulkanAPI::MemorySegment index_buffer;
		};
	}
}