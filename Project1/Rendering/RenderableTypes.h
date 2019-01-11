#pragma once
#include "Vulkan/TextureManager.h"
#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/DataTypes/PushBlocks.h"

// forward decleartions
namespace VulkanAPI
{
	class BufferManager;
}

namespace OmegaEngine
{
	// forward declerations
	class PipelineInterface;

	enum class RenderTypes
	{
		Mesh,
		Skybox,
		Ocean,
		Terrain,
		Count
	};

	// abstract base class
	class RenderableBase
	{
	public:

		RenderableBase(RenderTypes t) :
			type(t)
		{
		}

		virtual void build_graphics_pipeline(std::unique_ptr<PipelineInterface>& p_interface) = 0;

	protected:

		RenderTypes type;
	};

	// renderable object types
	class RenderableMesh : public RenderableBase
	{

	public:

		struct PrimitiveMesh
		{
			uint32_t index_offset;
			uint32_t index_count;

			VulkanAPI::MaterialPushBlock push_block;

			// descriptor sets for each mesh
			VulkanAPI::DescriptorSet decscriptor_set;

			// and the image views required to create the descriptor sets
			std::array<vk::ImageView, (int)PbrMaterials::Count> image_views;
		};
		
		RenderableMesh(RenderTypes type);

		static RenderPipeline create_mesh_pipeline(vk::Device device);

		void update_ssbo_buffer(std::unique_ptr<VulkanAPI::BufferManager>& buffer_man);

	private:

		// allocated GPU buffer 
		VulkanAPI::MemoryAllocator::SegmentInfo vertices;
		VulkanAPI::MemoryAllocator::SegmentInfo indicies;

		// primitive meshes - indices data and materials
		std::vector<PrimitiveMesh> primitives;

		// buffers
		VulkanAPI::Buffer ssbo;
	};
}