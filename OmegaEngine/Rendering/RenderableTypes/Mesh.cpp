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
#include "Rendering/RenderQueue.h"
#include "Managers/ComponentInterface.h"
#include "Threading/ThreadPool.h"

namespace OmegaEngine
{

	RenderableMesh::RenderableMesh(std::unique_ptr<ComponentInterface>& component_interface, 
									MeshManager::StaticMesh mesh, 
									MeshManager::PrimitiveMesh primitive) :
		RenderableBase(RenderTypes::Mesh)
	{
		
		// get the material for this primitive mesh from the manager
		auto& material_manager = component_interface->getManager<MaterialManager>();
		auto& mat = material_manager.get(primitive.materialId);

		// create the sorting key for this mesh
		sort_key = RenderQueue::create_sort_key(RenderStage::GBuffer, primitive.materialId, RenderTypes::Mesh);

		// fill out the data which will be used for rendering
		instance_data = new MeshInstance;
		MeshInstance* mesh_instance_data = reinterpret_cast<MeshInstance*>(instance_data);

		// index into the main buffer
		mesh_instance_data->vertex_buffer_offset = mesh.vertex_buffer_offset;
		mesh_instance_data->index_buffer_offset = mesh.index_buffer_offset;

		// per face indicies
		mesh_instance_data->index_sub_offset = primitive.indexBase;
		mesh_instance_data->index_count = primitive.indexCount;
			
		// materials
		mesh_instance_data->descr_set = mat.descr_set;
		mesh_instance_data->sampler = mat.sampler;

		// material push block
		
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
								void* instance,
								RenderInterface* render_interface,
								uint32_t thread)
	{
		// calculate offsets into dynamic buffer - these need to be in the same order as they are in the sets

		MeshInstance* instance_data = (MeshInstance*)instance;

		std::vector<uint32_t> dynamic_offsets 
		{
			instance_data->transform_dynamic_offset,
			instance_data->skinned_dynamic_offset
		};

		RenderPipeline& mesh_pipeline = render_interface->get_render_pipeline(RenderTypes::Mesh);

		cmd_buffer.secondary_bind_dynamic_descriptors(mesh_pipeline.pl_layout, mesh_pipeline.descr_set, VulkanAPI::PipelineType::Graphics, dynamic_offsets, thread);
		cmd_buffer.secondary_bind_push_block(mesh_pipeline.pl_layout, vk::ShaderStageFlagBits::eFragment, sizeof(instance_data->material_push_block), &instance_data->material_push_block, thread);

		// bind the material set to (set = 0)
		cmd_buffer.secondary_bind_descriptors(mesh_pipeline.pl_layout, instance_data->descr_set, VulkanAPI::PipelineType::Graphics, thread);

		vk::DeviceSize offset = {instance_data->vertex_buffer_offset};
		cmd_buffer.secondary_bind_vertex_buffer(instance_data->vertex_buffer, offset, thread);
		cmd_buffer.secondary_bind_index_buffer(instance_data->index_buffer, instance_data->index_sub_offset, thread);
		cmd_buffer.secondary_draw_indexed(instance_data->index_count, thread);
	}


	
}