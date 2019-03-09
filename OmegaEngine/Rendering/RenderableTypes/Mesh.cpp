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
	
	RenderInterface::ProgramState RenderableMesh::create_mesh_pipeline(vk::Device device, 
										std::unique_ptr<DeferredRenderer>& renderer, 
										std::unique_ptr<ComponentInterface>& component_interface)
	{
		
		RenderInterface::ProgramState state;

		// load shaders
		if (!state.shader.add(device, "model/model-vert.spv", VulkanAPI::StageType::Vertex, "model/model-frag.spv", VulkanAPI::StageType::Fragment)) {
			LOGGER_ERROR("Unable to create model shaders.");
		}

		// get pipeline layout and vertedx attributes by reflection of shader
		state.shader.descriptor_image_reflect(state.descr_layout, state.image_layout);
		state.shader.descriptor_buffer_reflect(state.descr_layout, state.buffer_layout);
		state.descr_layout.create(device);

		// we only want to init the uniform buffer sets, the material image samplers will be created by the materials themselves
		for (auto& buffer : state.buffer_layout) {
			state.descr_set.init(device, state.descr_layout.get_layout(buffer.set), state.descr_layout.get_pool(), buffer.set);
		}

		// sort out the descriptor sets - as long as we have initilaised the VkBuffers, we don't need to have filled the buffers yet
		// material sets will be created and owned by the actual material - note: these will always be set ZERO
		for (auto& layout : state.buffer_layout) {
			
			// the shader must use these identifying names for uniform buffers -
			if (layout.name == "CameraUbo") {
				auto& camera_manager = component_interface->getManager<CameraManager>();
				state.descr_set.write_set(layout.set, layout.binding, layout.type, camera_manager.get_ubo_buffer(), camera_manager.get_ubo_offset(), layout.range);
			}
			if (layout.name == "StaticMeshUbo") {
				auto& transform_manager = component_interface->getManager<TransformManager>();
				state.descr_set.write_set(layout.set, layout.binding, layout.type, transform_manager.get_mesh_ubo_buffer(), transform_manager.get_mesh_ubo_offset(), layout.range);
			}
			if (layout.name == "SkinnedUbo") {
				auto& transform_manager = component_interface->getManager<TransformManager>();
				state.descr_set.write_set(layout.set, layout.binding, layout.type, transform_manager.get_skinned_ubo_buffer(), transform_manager.get_skinned_ubo_offset(), layout.range);
			}
		}

		// we also need to send a referene to the material manager of the image descr set - the sets will be set at render time
		// it's assumed that the material combined image samplers will be set zero. TODO: Should add a more rigourous check
		component_interface->getManager<MaterialManager>().add_descr_layout(state.descr_layout.get_layout(state.image_layout[0][0].set), state.descr_layout.get_pool());

		state.shader.pipeline_layout_reflect(state.pl_layout);
		state.pl_layout.create(device, state.descr_layout.get_layout());

		// create the graphics pipeline
		state.shader.pipeline_reflection(state.pipeline);

		state.pipeline.set_depth_state(VK_TRUE, VK_TRUE);
		state.pipeline.add_dynamic_state(vk::DynamicState::eLineWidth);
		state.pipeline.set_topology(vk::PrimitiveTopology::eTriangleList);
		state.pipeline.add_colour_attachment(VK_FALSE, renderer->get_gbuffer_pass());
		state.pipeline.set_raster_front_face(vk::FrontFace::eCounterClockwise);
		state.pipeline.create(device, renderer->get_gbuffer_pass(), state.shader, state.pl_layout, VulkanAPI::PipelineType::Graphics);

		return state;
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

		RenderInterface::ProgramState& mesh_pipeline = render_interface->get_render_pipeline(RenderTypes::Mesh);

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