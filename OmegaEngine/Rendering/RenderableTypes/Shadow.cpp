#include "Shadow.h"
#include "Vulkan/Shader.h"
#include "Vulkan/BufferManager.h"
#include "Vulkan/CommandBuffer.h"
#include "Rendering/Renderers/RendererBase.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/RenderCommon.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Utility/logger.h"
#include "Objects/ObjectTypes.h"

namespace OmegaEngine
{

	RenderableShadow::RenderableShadow(RenderInterface* render_interface, ShadowComponent& component, RenderableMesh::MeshInstance* mesh_instance) :
		RenderableBase(RenderTypes::Skybox)
	{
		// fill out the data which will be used for rendering
		instance_data = new ShadowInstance;
		ShadowInstance* shadow_instance = reinterpret_cast<ShadowInstance*>(instance_data);
		
		// create the sorting key  TODO: actually implement this!
		sort_key = RenderQueue::create_sort_key(RenderStage::First, 0, RenderTypes::ShadowStatic);

		queue_type = QueueType::Shadow;

		// pointer to the mesh pipeline
		if (mesh_instance->type == MeshManager::MeshType::Static) {
			shadow_instance->state = render_interface->get_render_pipeline(RenderTypes::ShadowStatic).get();
		}
		else {
			shadow_instance->state = render_interface->get_render_pipeline(RenderTypes::ShadowDynamic).get();
		}

		// index into the main buffer - this is the vertex offset plus the offset into the actual memory segment
		shadow_instance->mesh_type = mesh_instance->type;
		shadow_instance->vertex_offset = mesh_instance->vertex_offset;
		shadow_instance->index_offset = mesh_instance->index_offset;
		shadow_instance->vertex_buffer = mesh_instance->vertex_buffer;
		shadow_instance->index_buffer = mesh_instance->index_buffer;
		shadow_instance->transform_dynamic_offset = mesh_instance->transform_dynamic_offset;
		shadow_instance->skinned_dynamic_offset = mesh_instance->skinned_dynamic_offset;

		shadow_instance->bias_clamp = component.bias_clamp;
		shadow_instance->bias_constant = component.bias_constant;
		shadow_instance->bias_slope = component.bias_slope;
	}


	RenderableShadow::~RenderableShadow()
	{
	}

	void RenderableShadow::create_shadow_pipeline(vk::Device& device,
		std::unique_ptr<RendererBase>& renderer,
		std::unique_ptr<VulkanAPI::BufferManager>& buffer_manager,
		std::unique_ptr<ProgramState>& state,
		MeshManager::MeshType type)
	{
		// load shaders - using the same shaders as the mesh, as we want to draw the vertices data, but aren't interested in colour information just depth
		if (type == MeshManager::MeshType::Static) {
			if (!state->shader.add(device, "model/model-vert.spv", VulkanAPI::StageType::Vertex)) {
				LOGGER_ERROR("Unable to create static shadow shaders.");
			}
		}
		else if (type == MeshManager::MeshType::Skinned) {
			if (!state->shader.add(device, "model/model_skinned-vert.spv", VulkanAPI::StageType::Vertex)) {
				LOGGER_ERROR("Unable to create skinned shadow shaders.");
			}
		}

		// get pipeline layout and vertedx attributes by reflection of shader
		state->shader.descriptor_image_reflect(state->descr_layout, state->image_layout);
		state->shader.descriptor_buffer_reflect(state->descr_layout, state->buffer_layout);
		state->descr_layout.create(device);
		state->descr_set.init(device, state->descr_layout);

		// sort out the descriptor sets - buffers
		for (auto& layout : state->buffer_layout) {
			
			// the shader must use these identifying names for uniform buffers -
			if (layout.name == "CameraUbo") {
				buffer_manager->enqueueDescrUpdate("Camera", &state->descr_set, layout.set, layout.binding, layout.type);
			}
			else if (layout.name == "Dynamic_StaticMeshUbo") {
				buffer_manager->enqueueDescrUpdate("Transform", &state->descr_set, layout.set, layout.binding, layout.type);
			}
			else if (layout.name == "Dynamic_SkinnedUbo") {
				buffer_manager->enqueueDescrUpdate("SkinnedTransform", &state->descr_set, layout.set, layout.binding, layout.type);
			}
		}

		state->shader.pipeline_layout_reflect(state->pl_layout);
		state->pl_layout.create(device, state->descr_layout.get_layout());

		// create the graphics pipeline
		state->shader.pipeline_reflection(state->pipeline);

		state->pipeline.set_depth_state(VK_TRUE, VK_FALSE);
		state->pipeline.set_raster_cull_mode(vk::CullModeFlagBits::eFront);
		state->pipeline.set_raster_front_face(vk::FrontFace::eClockwise);
		state->pipeline.set_topology(vk::PrimitiveTopology::eTriangleList);
		state->pipeline.add_dynamic_state(vk::DynamicState::eDepthBias);
		state->pipeline.create(device, renderer->get_shadow_pass(), state->shader, state->pl_layout, VulkanAPI::PipelineType::Graphics);
	}

	void RenderableShadow::create_shadow_pass(VulkanAPI::RenderPass& renderpass, VulkanAPI::Texture& image, 
		vk::Device& device, vk::PhysicalDevice& gpu, const vk::Format format, const uint32_t width, const uint32_t height)
	{
		
		// renderpass
		renderpass.init(device);
		renderpass.addAttachment(vk::ImageLayout::eDepthStencilAttachmentOptimal, format);
		renderpass.addSubpassDependency(VulkanAPI::DependencyTemplate::DepthStencil_Subpass_Top);
		renderpass.addSubpassDependency(VulkanAPI::DependencyTemplate::DepthStencil_Subpass_Bottom);
		renderpass.prepareRenderPass();

		// framebuffer
		image.create_empty_image(device, gpu, format, width, height,
			1, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);

		renderpass.prepareFramebuffer(image.get_image_view(), width, height, 1);
	}

	void RenderableShadow::render(VulkanAPI::SecondaryCommandBuffer& cmd_buffer, 
								void* instance)
	{
		ShadowInstance* instance_data = (ShadowInstance*)instance;

		ProgramState* state = instance_data->state;

		std::vector<uint32_t> dynamic_offsets{ instance_data->transform_dynamic_offset };
		if (instance_data->mesh_type == MeshManager::MeshType::Skinned) {
			dynamic_offsets.push_back(instance_data->skinned_dynamic_offset);
		}

		cmd_buffer.set_viewport();
		cmd_buffer.set_scissor();
		cmd_buffer.setDepthBias(instance_data->bias_constant, instance_data->bias_clamp, instance_data->bias_slope);
		cmd_buffer.bind_pipeline(state->pipeline);
		cmd_buffer.bind_dynamic_descriptors(state->pl_layout, state->descr_set, VulkanAPI::PipelineType::Graphics, dynamic_offsets);

		vk::DeviceSize offset = { instance_data->vertex_buffer.offset };
		cmd_buffer.bind_vertex_buffer(instance_data->vertex_buffer.buffer, offset);
		cmd_buffer.bind_index_buffer(instance_data->index_buffer.buffer, instance_data->index_buffer.offset);
		cmd_buffer.draw_indexed(instance_data->index_count);
	}
}
