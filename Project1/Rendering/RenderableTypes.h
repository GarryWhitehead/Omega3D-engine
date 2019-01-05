#pragma once
#include "Vulkan/TextureManager.h"
#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/DataTypes/PushBlocks.h"

namespace OmegaEngine
{

	enum class RenderTypes
	{
		Mesh,
		Skybox,
		Ocean,
		Terrain,
		Count
	};

	// abstract base class
	struct RenderableBase
	{

	};

	// renderable object types
	struct RenderableMesh : public RenderableBase
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

		// stuff for pipeline prep
		vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
	};
}