#include "Mesh.h"
#include "Vulkan/Vulkan_Global.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/CommandBuffer.h"
#include "Managers/CameraManager.h"
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
		RenderableBase(RenderTypes::Mesh, priority_key)
	{
		auto &mesh_man = comp_interface->getManager<MeshManager>();
		
		uint32_t mesh_index = obj.get_manager_index<MeshManager>();
		MeshManager::StaticMesh mesh = mesh_man.get_mesh(mesh_index);

		vertex_buffer_offset = mesh.vertex_buffer_offset;
		index_buffer_offset = mesh.index_buffer_offset;

		// sort index offsets and materials ready for rendering via the queue
		for (auto& prim : mesh.primitives) {

			MeshRenderInfo render_info;
			render_info.material_index = prim.materialId;

			render_info.vertex_buffer_offset = mesh.vertex_buffer_offset;
			render_info.index_buffer_offset = mesh.index_buffer_offset;

			render_info.index_offset = prim.indexBase;
			render_info.index_count = prim.indexCount;
			
			mesh_render_info.push_back(render_info);
		}
	}
	
	RenderPipeline RenderableMesh::create_mesh_pipeline(vk::Device device, 
										std::unique_ptr<DeferredRenderer>& renderer, 
										std::unique_ptr<ComponentInterface>& component_interface)
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
			
			auto& camera_manager = component_interface->getManager<CameraManager>();

			// the shader must use these identifying names -
			if (strcmp(layout.name, "CameraUbo") == 0) {
				
				render_pipeline.descr_set.write_set(layout.set, layout.binding, layout.type, camera_manager.get_ubo_buffer(), camera_manager.get_ubo_offset(), layout.range);
			}
			if (strcmp(layout.name, "StaticMeshUbo") == 0) {
				auto& mesh_manager = component_interface->getManager<MeshManager>();
				render_pipeline.descr_set.write_set(layout.set, layout.binding, layout.type, camera_manager.get_ubo_buffer(), camera_manager.get_ubo_offset(), layout.range);
			}
			if (strcmp(layout.name, "SkinnedUbo") == 0) {
				auto& mesh_manager = component_interface->getManager<MeshManager>();
				render_pipeline.descr_set.write_set(layout.set, layout.binding, layout.type, camera_manager.get_ubo_buffer(), camera_manager.get_ubo_offset(), layout.range);
			}
		}
		render_pipeline.descr_set.update_sets(device);

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


	void RenderableMesh::render(VulkanAPI::CommandBuffer& cmd_buffer, 
								MeshInstance* instance_data,
								std::unique_ptr<ComponentInterface>& component_interface,
								RenderInterface* render_interface)
	{	
		auto& trans_manager = component_interface->getManager<TransformManager>();

		// get the material for this primitive mesh from the manager
		auto& mat = component_interface->getManager<MaterialManager>().get(instance_data->material_index);

		// calculate offsets into dynamic buffer - these need to be in the same order as they are in the sets
		std::vector<uint32_t> dynamic_offsets 
		{
			instance_data->transform_dynamic_offset,
			instance_data->skinned_dynamic_offset
		};

		cmd_buffer.secondary_bind_dynamic_descriptors(mesh_pipeline.pl_layout, mesh_pipeline.descr_set, VulkanAPI::PipelineType::Graphics, dynamic_offsets, thread);
		cmd_buffer.secondary_bind_push_block(mesh_pipeline.pl_layout, vk::ShaderStageFlagBits::eFragment, sizeof(mesh_render_info[i].push_block), &mesh_render_info[i].push_block, thread);

		// bind the material set to (set = 0)
		cmd_buffer.secondary_bind_descriptors(mesh_pipeline.pl_layout, mat.descr_set, VulkanAPI::PipelineType::Graphics, thread);

		vk::DeviceSize offset = {vertex_buffer_offset};
		cmd_buffer.secondary_bind_vertex_buffer(vertices, offset, thread);
		cmd_buffer.secondary_bind_index_buffer(indicies, mesh_render_info[i].index_offset, thread);
		cmd_buffer.secondary_draw_indexed(mesh_render_info[i].index_count, thread);
	}


	
}