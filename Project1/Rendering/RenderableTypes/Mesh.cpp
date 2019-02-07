#include "Mesh.h"
#include "Vulkan/Vulkan_Global.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/CommandBuffer.h"
#include "Managers/MeshManager.h"
#include "Managers/TextureManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/TransformManager.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/DeferredRenderer.h"
#include "Managers/ComponentInterface.h"
#include "Threading/ThreadPool.h"

namespace OmegaEngine
{

	RenderableMesh::RenderableMesh(vk::Device device, std::unique_ptr<ComponentInterface>& comp_interface, Object& obj) :
		RenderableBase(RenderTypes::Mesh)
	{
		auto &mesh_man = comp_interface->getManager<MeshManager>();
		
		uint32_t mesh_index = obj.get_manager_index<MeshManager>();
		MeshManager::StaticMesh mesh = mesh_man.get_mesh(mesh_index);

		assert(!mesh.vertexBuffer.empty());
		assert(!mesh.indexBuffer.empty());
		
		VulkanAPI::MemoryAllocator &mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
		mem_alloc.mapDataToSegment(vertices, mesh.vertexBuffer.data(), mesh.vertexBuffer.size());
		mem_alloc.mapDataToSegment(indicies, mesh.indexBuffer.data(), mesh.indexBuffer.size());

		// sort index offsets and materials ready for rendering
		for (auto& prim : mesh.primitives) {

			PrimitiveData r_prim;

			VulkanAPI::Texture tex(VulkanAPI::TextureType::Normal);
			auto& mat = comp_interface->getManager<MaterialManager>().get(prim.materialId);

			// set up the push block 
			r_prim.push_block.create(mat);

			// map all of the pbr materials for this primitive mesh to the gpu
			for (uint8_t i = 0; i < (uint8_t)PbrMaterials::Count; ++i) {
				tex.map(comp_interface->getManager<TextureManager>().get_texture(mat.textures[i].image));

				// now update the decscriptor set with the texture info 
				r_prim.sampler->create(device, comp_interface->getManager<TextureManager>().get_sampler(mat.textures[i].sampler));
				r_prim.decscriptor_set.update_set(i, vk::DescriptorType::eSampler, r_prim.sampler->get_sampler(), tex.get_image_view(), vk::ImageLayout::eColorAttachmentOptimal);

				// indices data which will be used for creating the cmd buffers
				r_prim.index_offset = prim.indexBase;
				r_prim.index_count = prim.indexCount;
			}
		}
	}
	
	RenderPipeline RenderableMesh::create_mesh_pipeline(vk::Device device, std::unique_ptr<DeferredRenderer>& renderer)
	{
		RenderPipeline pipeline_info;
		
		// load shaders
		pipeline_info.shader.add(device, "model.vert", VulkanAPI::StageType::Vertex, "model.frag", VulkanAPI::StageType::Fragment);

		// get pipeline layout and vertedx attributes by reflection of shader
		std::vector<VulkanAPI::ShaderBufferLayout> buffer_layout;
		std::vector<VulkanAPI::ShaderImageLayout> image_layout;
		pipeline_info.shader.descriptor_image_reflect(pipeline_info.descr_layout, image_layout);
		pipeline_info.shader.descriptor_buffer_reflect(pipeline_info.descr_layout, buffer_layout);
		pipeline_info.shader.pipeline_layout_reflect(pipeline_info.pl_layout);

		// create the graphics pipeline
		pipeline_info.pipeline.set_depth_state(VK_TRUE, VK_TRUE);
		pipeline_info.pipeline.set_topology(vk::PrimitiveTopology::eTriangleList);
		pipeline_info.pipeline.add_colour_attachment(VK_FALSE, renderer->get_attach_count());
		pipeline_info.pipeline.set_raster_front_face(vk::FrontFace::eCounterClockwise);
		pipeline_info.pipeline.set_renderpass(renderer->get_renderpass());
		pipeline_info.pipeline.create(device, VulkanAPI::PipelineType::Graphics);

		return pipeline_info;
	}

	void RenderableMesh::update_ssbo_buffer(std::unique_ptr<VulkanAPI::BufferManager>& buffer_man)
	{
		
	}

	void RenderableMesh::render_threaded(VulkanAPI::CommandBuffer& cmd_buffer, RenderPipeline& mesh_pipeline, uint32_t start_index, uint32_t end_index, uint32_t thread)
	{
		// create a secondary command buffer for each thread. This also creates a thread-specific command pool 
		cmd_buffer.begin_secondary(thread);
		
		for (uint32_t i = start_index; i < end_index; ++i) {

			cmd_buffer.secondary_bind_descriptors(mesh_pipeline.pl_layout, primitives[i].decscriptor_set, VulkanAPI::PipelineType::Graphics, thread);
			cmd_buffer.secondary_bind_push_block(mesh_pipeline.pl_layout, vk::ShaderStageFlagBits::eFragment, sizeof(primitives[i].push_block), &primitives[i].push_block, thread);

			vk::DeviceSize offset = {0};
			cmd_buffer.secondary_bind_vertex_buffer(vertices, offset, thread);
			cmd_buffer.secondary_bind_index_buffer(indicies, primitives[i].index_offset, thread);
			cmd_buffer.secondary_draw_indexed(primitives[i].index_count, thread);
		}
	}

	void RenderableMesh::render(VulkanAPI::CommandBuffer& cmd_buffer, RenderPipeline& mesh_pipeline, ThreadPool& thread_pool, uint32_t thread_group_size, uint32_t num_threads)
	{
		
		uint32_t thread_count = 0;
		for (uint32_t i = 0; i < primitives.size(); i += thread_group_size) {

			// if we have no more threads left, then draw every mesh that is remaining
			if (i + 1 >= num_threads) {
		
				thread_pool.submitTask([&]() {
					render_threaded(cmd_buffer, mesh_pipeline, i, primitives.size(), thread_count);
				});
				break;
			}

			thread_pool.submitTask([&]() {
				render_threaded(cmd_buffer, mesh_pipeline, i, i + thread_group_size, thread_count);
			});

			++thread_count;
		}


	}
}