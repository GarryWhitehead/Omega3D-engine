#include "Mesh.h"
#include "Vulkan/Vulkan_Global.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/CommandBuffer.h"
#include "Managers/CameraManager.h"
#include "Managers/MeshManager.h"
#include "Managers/TextureManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/MeshManager.h"
#include "Managers/TransformManager.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/Renderers/DeferredRenderer.h"
#include "Rendering/RenderQueue.h"
#include "Managers/ComponentInterface.h"
#include "Threading/ThreadPool.h"

namespace OmegaEngine
{

	RenderableMesh::RenderableMesh(std::unique_ptr<ComponentInterface>& component_interface, 
									MeshManager::StaticMesh mesh, 
									MeshManager::PrimitiveMesh primitive) :
		RenderableBase(RenderTypes::StaticMesh)
	{
		
		// get the material for this primitive mesh from the manager
		auto& material_manager = component_interface->getManager<MaterialManager>();
		auto& mesh_manager = component_interface->getManager<MeshManager>();

		auto& mat = material_manager.get(primitive.materialId);

		// create the sorting key for this mesh
		sort_key = RenderQueue::create_sort_key(RenderStage::GBuffer, primitive.materialId, RenderTypes::StaticMesh);

		// fill out the data which will be used for rendering
		instance_data = new MeshInstance;
		MeshInstance* mesh_instance_data = reinterpret_cast<MeshInstance*>(instance_data);

		// skinned ior non-skinned mesh?
		mesh_instance_data->type = mesh.type;

		// index into the main buffer - this is the vertex offset plus the offset into the actual memory segment
		mesh_instance_data->vertex_buffer_offset = mesh.vertex_buffer_offset + mesh_manager.get_vertex_buffer_offset(mesh.type);
		mesh_instance_data->index_buffer_offset = mesh.index_buffer_offset + mesh_manager.get_index_buffer_offset(mesh.type);

		// actual vulkan buffers
		mesh_instance_data->vertex_buffer = mesh_manager.get_vertex_buffer(mesh.type);
		mesh_instance_data->index_buffer = mesh_manager.get_index_buffer(mesh.type);
		
		// per face indicies
		mesh_instance_data->index_sub_offset = primitive.indexBase;
		mesh_instance_data->index_count = primitive.indexCount;
			
		// materials
		mesh_instance_data->descr_set = mat.descr_set;
		mesh_instance_data->sampler = mat.sampler;

		// material push block
		mesh_instance_data->material_push_block.baseColorFactor = mat.factors.baseColour;
		mesh_instance_data->material_push_block.metallicFactor = mat.factors.metallic;
		mesh_instance_data->material_push_block.roughnessFactor = mat.factors.roughness;
		mesh_instance_data->material_push_block.emissiveFactor = mat.factors.emissive;
		mesh_instance_data->material_push_block.specularFactor = mat.factors.specular;
		mesh_instance_data->material_push_block.diffuseFactor = mat.factors.diffuse;
		mesh_instance_data->material_push_block.alphaMask = mat.factors.alphaMask;
		mesh_instance_data->material_push_block.alphaMaskCutoff = mat.factors.alphaMaskCutOff;
		mesh_instance_data->material_push_block.haveBaseColourMap = mat.texture_state[(int)PbrMaterials::BaseColor] ? 1 : 0;
		mesh_instance_data->material_push_block.haveMrMap = mat.texture_state[(int)PbrMaterials::MetallicRoughness] ? 1 : 0;
		mesh_instance_data->material_push_block.haveNormalMap = mat.texture_state[(int)PbrMaterials::Normal] ? 1 : 0;
		mesh_instance_data->material_push_block.haveAoMap = mat.texture_state[(int)PbrMaterials::Occlusion] ? 1 : 0;
		mesh_instance_data->material_push_block.haveEmissiveMap = mat.texture_state[(int)PbrMaterials::Emissive] ? 1 : 0;
		mesh_instance_data->material_push_block.usingSpecularGlossiness = mat.usingSpecularGlossiness ? 1 : 0;
		mesh_instance_data->material_push_block.baseColourUvSet = mat.uvSets.baseColour;
		mesh_instance_data->material_push_block.metallicRoughnessUvSet = mat.uvSets.metallicRoughness;
		mesh_instance_data->material_push_block.normalUvSet = mat.uvSets.normal;
		mesh_instance_data->material_push_block.occlusionUvSet = mat.uvSets.occlusion;
		mesh_instance_data->material_push_block.emissiveUvSet = mat.uvSets.emissive;

		if (mesh_instance_data->material_push_block.usingSpecularGlossiness) {
			mesh_instance_data->material_push_block.metallicRoughnessUvSet = mat.uvSets.specularGlossiness;
			mesh_instance_data->material_push_block.baseColourUvSet = mat.uvSets.diffuse;
		}
	}
	
	RenderInterface::ProgramState RenderableMesh::create_mesh_pipeline(vk::Device device, 
										std::unique_ptr<RendererBase>& renderer, 
										std::unique_ptr<ComponentInterface>& component_interface,
										MeshManager::MeshType type)
	{
		
		RenderInterface::ProgramState state;

		// load shaders
		if (type == MeshManager::MeshType::Static) {
			if (!state.shader.add(device, "model/model-vert.spv", VulkanAPI::StageType::Vertex, "model/model-frag.spv", VulkanAPI::StageType::Fragment)) {
				LOGGER_ERROR("Unable to create static model shaders.");
			}
		}
		else if (type == MeshManager::MeshType::Skinned) {
			if (!state.shader.add(device, "model/model_skinned-vert.spv", VulkanAPI::StageType::Vertex, "model/model-frag.spv", VulkanAPI::StageType::Fragment)) {
				LOGGER_ERROR("Unable to create skinned model shaders.");
			}
		}

		// get pipeline layout and vertedx attributes by reflection of shader
		state.shader.descriptor_image_reflect(state.descr_layout, state.image_layout);
		state.shader.descriptor_buffer_reflect(state.descr_layout, state.buffer_layout);
		state.descr_layout.create(device, TOTAL_MATERIAL_SETS);

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
			if (layout.name == "Dynamic_StaticMeshUbo") {
				auto& transform_manager = component_interface->getManager<TransformManager>();
				state.descr_set.write_set(layout.set, layout.binding, layout.type, transform_manager.get_mesh_ubo_buffer(), transform_manager.get_mesh_ubo_offset(), layout.range);
			}
			if (layout.name == "Dynamic_SkinnedUbo") {
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
		state.pipeline.set_raster_cull_mode(vk::CullModeFlagBits::eBack);
		state.pipeline.set_raster_front_face(vk::FrontFace::eClockwise);
		state.pipeline.set_topology(vk::PrimitiveTopology::eTriangleList);
		state.pipeline.add_colour_attachment(VK_FALSE, renderer->get_first_pass());
		state.pipeline.create(device, renderer->get_first_pass(), state.shader, state.pl_layout, VulkanAPI::PipelineType::Graphics);

		return state;
	}


	void RenderableMesh::render(VulkanAPI::CommandBuffer& cmd_buffer, 
								void* instance,
								RenderInterface* render_interface)
	{
		// calculate offsets into dynamic buffer - these need to be in the same order as they are in the sets

		MeshInstance* instance_data = (MeshInstance*)instance;

		std::vector<uint32_t> dynamic_offsets { instance_data->transform_dynamic_offset };
		if (instance_data->type == MeshManager::MeshType::Skinned) {
			dynamic_offsets.push_back(instance_data->skinned_dynamic_offset);
		}

		RenderInterface::ProgramState mesh_pipeline;
		if (instance_data->type == MeshManager::MeshType::Static) {
			mesh_pipeline = render_interface->get_render_pipeline(RenderTypes::StaticMesh);
		}
		else if (instance_data->type == MeshManager::MeshType::Skinned) {
			mesh_pipeline = render_interface->get_render_pipeline(RenderTypes::SkinnedMesh);
		}
		else {
			LOGGER_ERROR("Unsupported mesh type!");
		}

		// merge the material set with the mesh ubo sets
		std::vector<vk::DescriptorSet> material_set = instance_data->descr_set.get();
		std::vector<vk::DescriptorSet> mesh_set = mesh_pipeline.descr_set.get();
		material_set.insert(material_set.end(), mesh_set.begin(), mesh_set.end());

		cmd_buffer.set_viewport();
		cmd_buffer.set_scissor();
		cmd_buffer.bind_pipeline(mesh_pipeline.pipeline);
		cmd_buffer.bind_dynamic_descriptors(mesh_pipeline.pl_layout, material_set, VulkanAPI::PipelineType::Graphics, dynamic_offsets);
		cmd_buffer.bind_push_block(mesh_pipeline.pl_layout, vk::ShaderStageFlagBits::eFragment, sizeof(MeshInstance::MaterialPushBlock), &instance_data->material_push_block);

		vk::DeviceSize offset = { instance_data->vertex_buffer_offset };
		cmd_buffer.bind_vertex_buffer(instance_data->vertex_buffer, offset);
		cmd_buffer.bind_index_buffer(instance_data->index_buffer, instance_data->index_buffer_offset + instance_data->index_sub_offset);
		cmd_buffer.draw_indexed(instance_data->index_count);
	}

}