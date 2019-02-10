#pragma once
#include "RenderableBase.h"
#include "Vulkan/MemoryAllocator.h"
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
	class DeferredRenderer;
	class ComponentInterface;
	class ThreadPool;
	struct RenderPipeline;
	class Object;

	// renderable object types
	class RenderableMesh : public RenderableBase
	{

	public:

		struct PrimitiveData
		{
			uint32_t index_offset;
			uint32_t index_count;

			VulkanAPI::MaterialPushBlock push_block;

			// descriptor sets for each mesh
			VulkanAPI::DescriptorSet decscriptor_set;
			std::unique_ptr<VulkanAPI::Sampler> sampler;
		};
		
		RenderableMesh(vk::Device& device, std::unique_ptr<ComponentInterface>& comp_interface, Object& obj);

		void render(VulkanAPI::CommandBuffer& cmd_buffer, RenderPipeline& mesh_pipeline, ThreadPool& thread_pool, uint32_t thread_group_size, uint32_t num_threads);
		void render_threaded(VulkanAPI::CommandBuffer& cmd_buffer, RenderPipeline& mesh_pipeline, uint32_t start_index, uint32_t end_index, uint32_t thread);

		static RenderPipeline create_mesh_pipeline(vk::Device device, std::unique_ptr<DeferredRenderer>& renderer);	// TODO: this will need thinking about when other renderer types are added

		void update_ssbo_buffer(std::unique_ptr<VulkanAPI::BufferManager>& buffer_man);

	private:

		// allocated GPU buffer 
		VulkanAPI::MemorySegment vertices;
		VulkanAPI::MemorySegment indicies;

		// primitive meshes - indices data and materials
		std::vector<PrimitiveData> primitives;
	};
}