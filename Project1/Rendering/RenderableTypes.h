#pragma once
#include "Vulkan/TextureManager.h"
#include "Vulkan/MemoryAllocator.h"

namespace OmegaEngine
{

	// abstract base class
	struct RenderableType
	{

	};

	// renderable object types
	struct RenderableMesh : public RenderableType
	{
		struct PrimitiveMesh
		{
			uint32_t index_offset;
			uint32_t index_count;

			VulkanAPI::MaterialPushBlock push_block;

			std::array<vk::ImageView, (int)PbrMaterials::Count> image_views;
	
		};
		
		// allocated GPU buffer 
		VulkanAPI::MemoryAllocator::SegmentInfo vertices;
		VulkanAPI::MemoryAllocator::SegmentInfo indicies;

		// primitive meshes - indices data and materials
		std::vector<PrimitiveMesh> primitives;
	};
}