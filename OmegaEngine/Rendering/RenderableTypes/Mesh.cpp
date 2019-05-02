#include "Mesh.h"
#include "Vulkan/BufferManager.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/VkTextureManager.h"
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

	RenderableMesh::RenderableMesh(vk::Device& device,
									std::unique_ptr<ComponentInterface>& component_interface,
									std::unique_ptr<VulkanAPI::BufferManager>& buffer_manager, 
									std::unique_ptr<VulkanAPI::VkTextureManager>& texture_manager,
									MeshManager::StaticMesh mesh, 
									MeshManager::PrimitiveMesh primitive,
									Object& obj, 
									RenderInterface* render_interface) :
		RenderableBase(RenderTypes::StaticMesh)
	{
		
		// get the material for this primitive mesh from the manager
		auto& material_manager = component_interface->getManager<MaterialManager>();
		auto& mat = material_manager.get(primitive.materialId);

		// create the sorting key for this mesh
		sort_key = RenderQueue::create_sort_key(RenderStage::First, primitive.materialId, RenderTypes::StaticMesh);

		// fill out the data which will be used for rendering
		instance_data = new MeshInstance;
		MeshInstance* mesh_instance_data = reinterpret_cast<MeshInstance*>(instance_data);

		// skinned ior non-skinned mesh?
		mesh_instance_data->type = mesh.type;

		// pointer to the mesh pipeline
		if (mesh.type == MeshManager::MeshType::Static) {
			mesh_instance_data->state = render_interface->get_render_pipeline(RenderTypes::StaticMesh).get();
		}
		else {
			mesh_instance_data->state = render_interface->get_render_pipeline(RenderTypes::SkinnedMesh).get();
		}

		// index into the main buffer - this is the vertex offset plus the offset into the actual memory segment
		mesh_instance_data->vertex_offset = mesh.vertex_buffer_offset;
		mesh_instance_data->index_offset = mesh.index_buffer_offset;

		// actual vulkan buffers
		if (mesh.type == MeshManager::MeshType::Static) {
			mesh_instance_data->vertex_buffer = buffer_manager->get_buffer("StaticVertices");
		}
		else {
			mesh_instance_data->vertex_buffer = buffer_manager->get_buffer("SkinnedVertices");
		}
		
		mesh_instance_data->index_buffer = buffer_manager->get_buffer("Indices");
		
		// dynamic buffer offset to point at the transform matrix for this mesh
		auto& transform_manager = component_interface->getManager<TransformManager>();
		mesh_instance_data->transform_dynamic_offset = transform_manager.get_transform_offset(obj.get_id());
		if (mesh.type == MeshManager::MeshType::Skinned) {
			mesh_instance_data->skinned_dynamic_offset = transform_manager.get_skinned_offset(obj.get_id());
		}
		
		// per face indicies
		mesh_instance_data->index_primitive_offset = primitive.indexBase;
		mesh_instance_data->index_primitive_count = primitive.indexCount;
			
		// materials 
		// create a descriptor set for this material
		VulkanAPI::VkTextureManager::TextureLayoutInfo layout_info;
		if (mesh.type == MeshManager::MeshType::Static) {
			layout_info = texture_manager->get_texture_descr_layout("Mesh");
		}
		else if (mesh.type == MeshManager::MeshType::Skinned) {
			layout_info = texture_manager->get_texture_descr_layout("SkinnedMesh");
		}

		mesh_instance_data->descr_set.init(device, *layout_info.layout, layout_info.set_num); 
		texture_manager->update_material_descr_set(mesh_instance_data->descr_set, mat.name, layout_info.set_num);

		// material push block
		mesh_instance_data->material_push_block.baseColorFactor = mat.factors.baseColour;
		mesh_instance_data->material_push_block.metallicFactor = mat.factors.metallic;
		mesh_instance_data->material_push_block.roughnessFactor = mat.factors.roughness;
		mesh_instance_data->material_push_block.emissiveFactor = mat.factors.emissive;
		mesh_instance_data->material_push_block.specularFactor = mat.factors.specular;
		mesh_instance_data->material_push_block.diffuseFactor = OEMaths::vec3f{ mat.factors.diffuse.x, mat.factors.diffuse.y, mat.factors.diffuse.z };
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
	
	void RenderableMesh::create_mesh_pipeline(vk::Device& device, 
										std::unique_ptr<RendererBase>& renderer, 
										std::unique_ptr<VulkanAPI::BufferManager>& buffer_manager,
										std::unique_ptr<VulkanAPI::VkTextureManager>& texture_manager,
										MeshManager::MeshType type,
										std::unique_ptr<RenderInterface::ProgramState>& state)
	{
		// load shaders
		if (type == MeshManager::MeshType::Static) {
			if (!state->shader.add(device, "model/model-vert.spv", VulkanAPI::StageType::Vertex, "model/model-frag.spv", VulkanAPI::StageType::Fragment)) {
				LOGGER_ERROR("Unable to create static model shaders.");
			}
		}
		else if (type == MeshManager::MeshType::Skinned) {
			if (!state->shader.add(device, "model/model_skinned-vert.spv", VulkanAPI::StageType::Vertex, "model/model-frag.spv", VulkanAPI::StageType::Fragment)) {
				LOGGER_ERROR("Unable to create skinned model shaders.");
			}
		}

		// get pipeline layout and vertedx attributes by reflection of shader
		state->shader.descriptor_image_reflect(state->descr_layout, state->image_layout);
		state->shader.descriptor_buffer_reflect(state->descr_layout, state->buffer_layout);
		state->descr_layout.create(device, TOTAL_MATERIAL_SETS);

		// we only want to init the uniform buffer sets, the material image samplers will be created by the materials themselves
		for (auto& buffer : state->buffer_layout) {
			state->descr_set.init(device, state->descr_layout.get_layout(buffer.set), state->descr_layout.get_pool(), buffer.set);
		}

		// sort out the descriptor sets - as long as we have initilaised the VkBuffers, we don't need to have filled the buffers yet
		// material sets will be created and owned by the actual material - note: these will always be set ZERO
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

		// inform the texture manager the layout of textures associated with the mesh shader
		// TODO : automate this somehow rather than hard coded values
		const uint8_t material_set = 0;
		if (type == MeshManager::MeshType::Static) {
			texture_manager->bind_textures_to_layout("Mesh", &state->descr_layout, material_set);
		}
		else if (type == MeshManager::MeshType::Skinned) {
			texture_manager->bind_textures_to_layout("SkinnedMesh", &state->descr_layout, material_set);
		}

		state->shader.pipeline_layout_reflect(state->pl_layout);
		state->pl_layout.create(device, state->descr_layout.get_layout());

		// create the graphics pipeline
		state->shader.pipeline_reflection(state->pipeline);

		state->pipeline.set_depth_state(VK_TRUE, VK_TRUE);
		state->pipeline.set_raster_cull_mode(vk::CullModeFlagBits::eBack);
		state->pipeline.set_raster_front_face(vk::FrontFace::eClockwise);
		state->pipeline.set_topology(vk::PrimitiveTopology::eTriangleList);
		state->pipeline.add_colour_attachment(VK_FALSE, renderer->get_first_pass());
		state->pipeline.create(device, renderer->get_first_pass(), state->shader, state->pl_layout, VulkanAPI::PipelineType::Graphics);
	}


	void RenderableMesh::render(VulkanAPI::SecondaryCommandBuffer& cmd_buffer, 
								void* instance)
	{
		MeshInstance* instance_data = (MeshInstance*)instance;

		std::vector<uint32_t> dynamic_offsets { instance_data->transform_dynamic_offset };
		if (instance_data->type == MeshManager::MeshType::Skinned) {
			dynamic_offsets.push_back(instance_data->skinned_dynamic_offset);
		}

		// merge the material set with the mesh ubo sets
		RenderInterface::ProgramState* state = instance_data->state;
		std::vector<vk::DescriptorSet> material_set = state->descr_set.get();
		std::vector<vk::DescriptorSet> mesh_set = state->descr_set.get();
		material_set.insert(material_set.end(), mesh_set.begin(), mesh_set.end());

		cmd_buffer.set_viewport();
		cmd_buffer.set_scissor();
		cmd_buffer.bind_pipeline(state->pipeline);
		cmd_buffer.bind_dynamic_descriptors(state->pl_layout, material_set, VulkanAPI::PipelineType::Graphics, dynamic_offsets);
		cmd_buffer.bind_push_block(state->pl_layout, vk::ShaderStageFlagBits::eFragment, sizeof(MeshInstance::MaterialPushBlock), &instance_data->material_push_block);

		vk::DeviceSize offset = { instance_data->vertex_buffer.offset };
		cmd_buffer.bind_vertex_buffer(instance_data->vertex_buffer.buffer, offset);
		cmd_buffer.bind_index_buffer(instance_data->index_buffer.buffer, instance_data->index_buffer.offset + instance_data->index_offset + instance_data->index_primitive_offset);
		cmd_buffer.draw_indexed(instance_data->index_primitive_count);
	}

}