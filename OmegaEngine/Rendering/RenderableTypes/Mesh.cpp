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

	RenderableMesh::RenderableMesh(std::unique_ptr<ComponentInterface>& comp_interface, Object& obj) :
		RenderableBase(RenderTypes::Mesh)
	{
		auto &mesh_man = comp_interface->getManager<MeshManager>();
		
		uint32_t mesh_index = obj.get_manager_index<MeshManager>();
		MeshManager::StaticMesh mesh = mesh_man.get_mesh(mesh_index);

		vertex_buffer_offset = mesh.vertex_buffer_offset;
		index_buffer_offset = mesh.index_buffer_offset;

		// sort index offsets and materials ready for rendering
		for (auto& prim : mesh.primitives) {

			MeshRenderInfo render_info;
			render_info.material_index = prim.materialId;

			render_info.index_offset = prim.index_offset;
			render_info.index_count = prim.index_count;
			
			mesh_render_info.push_back(render_info);
		}
	}
	
	RenderPipeline RenderableMesh::create_mesh_pipeline(vk::Device device, 
												std::unique_ptr<DeferredRenderer>& renderer, 
												std::unique_ptr<ComponentInterface>& interface)
	{
		
		RenderPipeline render_pipeline;

		// load shaders
		render_pipeline.shader.add(device, "model.vert", VulkanAPI::StageType::Vertex, "model.frag", VulkanAPI::StageType::Fragment);

		// get pipeline layout and vertedx attributes by reflection of shader
		render_pipeline.shader.descriptor_image_reflect(render_pipeline.descr_layout, render_pipeline.image_layout);
		render_pipeline.shader.descriptor_buffer_reflect(render_pipeline.descr_layout, render_pipeline.buffer_layout);
		render_pipeline.descr_layout.create(device);
		
		// sort out the descriptor sets - as long as we have initilaised the VkBuffers, we don't need to have filled the buffers yet
		// material sets will be created and owned by the actual material - note: these will always be set ZERO
		for (auto& layout : render_pipeline.buffer_layout) {
			
			// the shader must use these identifying names -
			if (strcmp(layout.name, "CameraUbo") == 0) {
				auto& camera_manager = interface->get_manager<CameraManager>();
				render_pipeline.descr_set.write_set(layout.set, layout.binding, layout.type, camera_manager->get_ubo_buffer(), camera_manager->get_ubo_offset(), layout.range);
			}
			if (strcmp(layout.name, "StaticMeshUbo") == 0) {
				auto& mesh_manager = interface->get_manager<MeshManager>();
				render_pipeline.descr_set.write_set(layout.set, layout.binding, layout.type, camera_manager->get_ubo_buffer(), camera_manager->get_ubo_offset(), layout.range);
			}
			if (strcmp(layout.name, "SkinnedUbo") == 0) {
				auto& mesh_manager = interface->get_manager<MeshManager>();
				render_pipeline.descr_set.write_set(layout.set, layout.binding, layout.type, camera_manager->get_ubo_buffer(), camera_manager->get_ubo_offset(), layout.range);
			}
		}
		rener_pipeline.descr_set.update_sets(device);

		render_pipeline.shader.pipeline_layout_reflect(render_pipeline.pl_layout);
		render_pipeline.pl_layout.create(device, render_pipeline.descr_layout.get_layout(), RenderTypes::Mesh);

		// create the graphics pipeline
		render_pipeline.pipeline.set_depth_state(VK_TRUE, VK_TRUE);
		render_pipeline.pipeline.set_topology(vk::PrimitiveTopology::eTriangleList);
		render_pipeline.pipeline.add_colour_attachment(VK_FALSE, renderer->get_attach_count());
		render_pipeline.pipeline.set_raster_front_face(vk::FrontFace::eCounterClockwise);
		render_pipeline.pipeline.set_renderpass(renderer->get_renderpass());
		render_pipeline.pipeline.add_layout(render_pipeline.pl_layout.get());
		render_pipeline.pipeline.create(device, VulkanAPI::PipelineType::Graphics);

		return render_pipeline;
	}


	void RenderableMesh::render_threaded(VulkanAPI::CommandBuffer& cmd_buffer, 
											RenderPipeline& mesh_pipeline, 
											std::unique_ptr<ComponentInterface>& interface,
											uint32_t start_index, uint32_t end_index, 
											uint32_t thread)
	{
		// create a secondary command buffer for each thread. This also creates a thread-specific command pool 
		cmd_buffer.begin_secondary(thread);
		
		auto& trans_manager = interface->getManager<TransformManager>();

		for (uint32_t i = start_index; i < end_index; ++i) {
			
			// get the material for this primitive mesh from the manager
			auto& mat = interface->getManager<MaterialManager>().get(primitives[i].material_index);

			// calculate offsets into dynamic buffer - these need to be in the same order as they are in the sets
			std::vector<uint32_t> dynamic_offsets 
			{
				i * trans_manager-<get_transform_dynamic_offset(),
				i * trans_manager-<get_skinned_dynamic_offset(),
			};

			cmd_buffer.secondary_bind_dynamic_descriptors(mesh_pipeline.pl_layout, mesh_pipeline.descr_set, VulkanAPI::PipelineType::Graphics, dynamic_offsets, thread);
			cmd_buffer.secondary_bind_push_block(mesh_pipeline.pl_layout, vk::ShaderStageFlagBits::eFragment, sizeof(primitives[i].push_block), &primitives[i].push_block, thread);

			// bind the material set to (set = 0)
			cmd_buffer.secondary_bind_descriptors(mesh_pipeline.pl_layout, mat.descr_set, VulkanAPI::PipelineType::Graphics, thread);

			vk::DeviceSize offset = {vertex_buffer_offset};
			cmd_buffer.secondary_bind_vertex_buffer(vertices, offset, thread);
			cmd_buffer.secondary_bind_index_buffer(indicies, primitives[i].index_offset, thread);
			cmd_buffer.secondary_draw_indexed(primitives[i].index_count, thread);
		}
	}


	void RenderableMesh::render(VulkanAPI::CommandBuffer& cmd_buffer, 
								RenderPipeline& mesh_pipeline, 
								ThreadPool& thread_pool, 
								uint32_t thread_group_size, 
								uint32_t num_threads)
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