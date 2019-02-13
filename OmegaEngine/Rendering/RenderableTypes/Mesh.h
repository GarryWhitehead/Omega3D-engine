#pragma once
#include "RenderableBase.h"
#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/DataTypes/PushBlocks.h"

// forward decleartions
namespace VulkanAPI
{
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
		
		// priority key for meshes required for sorting
		const uint32_t priority_key = 1;

		// render info for all mesh primitive faces 
		struct MeshRenderInfo
		{
			// face indicies data
			uint32_t index_offset;
			uint32_t index_count;

			VulkanAPI::MaterialPushBlock push_block;

			// descriptor sets for each mesh
			int32_t material_index = -1;
		};
		
		RenderableMesh(std::unique_ptr<ComponentInterface>& comp_interface, Object& obj);

		void render(VulkanAPI::CommandBuffer& cmd_buffer, 
					RenderPipeline& mesh_pipeline, 
					std::unique_ptr<ComponentInterface>& component_interface,
					ThreadPool& thread_pool, 
					uint32_t thread_group_size, 
					uint32_t num_threads) override;

		void render_threaded(VulkanAPI::CommandBuffer& cmd_buffer,
							RenderPipeline& mesh_pipeline, 
							std::unique_ptr<ComponentInterface>& component_interface,
							uint32_t start_index, uint32_t end_index, 
							uint32_t thread);

		static RenderPipeline create_mesh_pipeline(vk::Device device, 
													std::unique_ptr<DeferredRenderer>& renderer, 
													std::unique_ptr<ComponentInterface>& component_interface);	

	private:

		// primitive meshes - indices data and material index
		std::vector<MeshRenderInfo> mesh_render_info;

		// offsets into the mapped buffer
		uint32_t vertex_buffer_offset;
		uint32_t index_buffer_offset;
	};
}