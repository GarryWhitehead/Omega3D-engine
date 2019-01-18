#pragma once
#include "RenderableBase.h"
#include "Vulkan/TextureManager.h"
#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/DataTypes/PushBlocks.h"

// forward decleartions
namespace VulkanAPI
{
	class BufferManager;
	class Sampler;
	class CommandBuffer;
}

namespace OmegaEngine
{
	// forward declerations
	class PipelineInterface;
	class Renderer;
	class ComponentInterface;
	class ThreadPool;

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
			std::unique_ptr<VulkanAPI::Sampler> sampler;
		};
		
		RenderableMesh(std::unique_ptr<ComponentInterface>& comp_interface, Object& obj);

		void render(VulkanAPI::CommandBuffer& cmd_buffer, RenderPipeline& mesh_pipeline, ThreadPool& thread_pool);

		static RenderPipeline create_mesh_pipeline(vk::Device device, std::unique_ptr<Renderer>& renderer);

		void update_ssbo_buffer(std::unique_ptr<VulkanAPI::BufferManager>& buffer_man);

	private:

		// allocated GPU buffer 
		VulkanAPI::MemorySegment vertices;
		VulkanAPI::MemorySegment indicies;

		// primitive meshes - indices data and materials
		std::vector<PrimitiveMesh> primitives;

		// buffers
		VulkanAPI::Buffer ssbo;
	};
}