#include "Mesh.h"
#include "Vulkan/BufferManager.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/CommandBuffer.h"
#include "Managers/MeshManager.h"
#include "Managers/TextureManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/TransformManager.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/Renderer.h"
#include "ComponentInterface/ComponentInterface.h"

namespace OmegaEngine
{

	RenderableMesh::RenderableMesh(std::unique_ptr<ComponentInterface>& comp_interface, Object& obj) :
		RenderableBase(RenderTypes::Mesh)
	{
		auto &transform_man = comp_interface->getManager<TransformManager>();
		auto &mesh_man = comp_interface->getManager<MeshManager>();
		MeshManager::StaticMesh mesh = mesh_man->getStaticMesh(obj);

		assert(!mesh.vertexBuffer.empty());
		assert(!mesh.indexBuffer.empty());

		vk_interface->get_mem_alloc()->mapDataToSegment<MeshManager::Vertex>(vertices, vertexBuffer);
		vk_interface->get_mem_alloc()->mapDataToSegment<uint32_t>(indicies, indexBuffer);

		// sort index offsets and materials ready for rendering
		for (auto& prim : mesh.primitives) {

			PrimitiveMesh r_prim;

			VulkanAPI::Texture tex(VulkanAPI::TextureType::Normal);
			auto& mat = comp_interface->getManager<MaterialManager>()->get(prim.materialId);

			// set up the push block 
			r_prim.push_block.create(mat);

			// map all of the pbr materials for this primitive mesh to the gpu
			for (uint8_t i = 0; i < (uint8_t)PbrMaterials::Count; ++i) {
				tex.map(comp_interface->getManager<TextureManager>()->get_texture(mat.textures[i].image), vk_interface->get_mem_alloc());

				// now update the decscriptor set with the texture info 
				r_prim.sampler = std::make_unique<VulkanAPI::Sampler>(comp_interface->getManager<TextureManager>()->get_sampler(mat.textures[i].sampler));
				r_prim.decscriptor_set.update_set(i, vk::DescriptorType::eSampler, r_prim.sampler->get_sampler(), tex.get_image_view(), vk::ImageLayout::eColorAttachmentOptimal);

				// indices data which will be used for creating the cmd buffers
				r_prim.index_offset = prim.indexBase;
				r_prim.index_count = prim.indexCount;
			}
		}
	}
	
	RenderPipeline RenderableMesh::create_mesh_pipeline(vk::Device device, std::unique_ptr<Renderer>& renderer)
	{
		RenderPipeline pipeline_info;
		
		// load shaders
		pipeline_info.shader.add(device, "model.vert", VulkanAPI::StageType::Vertex, "model.frag", VulkanAPI::StageType::Fragment);

		// get pipeline layout and vertedx attributes by reflection of shader
		std::vector<VulkanAPI::ShaderBufferLayout> buffer_layout;
		std::vector<VulkanAPI::ShaderImageLayout> image_layout;
		pipeline_info.shader.reflection(VulkanAPI::StageType::Vertex, *pipeline_info.descr_layout, pipeline_info.pl_layout, pipeline_info.pipeline, image_layout, buffer_layout);
		pipeline_info.shader.reflection(VulkanAPI::StageType::Fragment, *pipeline_info.descr_layout, pipeline_info.pl_layout, pipeline_info.pipeline, image_layout, buffer_layout);

		// create the graphics pipeline
		pipeline_info.pipeline.set_depth_state(VK_TRUE, VK_TRUE);
		pipeline_info.pipeline.set_topology(vk::PrimitiveTopology::eTriangleList);
		pipeline_info.pipeline.add_colour_attachment(VK_FALSE, renderer->get_attach_count());
		pipeline_info.pipeline.set_raster_front_face(vk::FrontFace::eCounterClockwise);
		pipeline_info.pipeline.set_renderpass(renderer->get_renderpass());
		pipeline_info.pipeline.create();
	}

	void RenderableMesh::update_ssbo_buffer(std::unique_ptr<VulkanAPI::BufferManager>& buffer_man)
	{
		
	}

	void RenderableMesh::render(VulkanAPI::CommandBuffer& cmd_buffer, RenderPipeline& mesh_pipeline, ThreadPool& thread_pool)
	{
		// each mesh will have a secondary cmd buffer which will be dispatched on seperate threads
		// each primitive will be drawn on the same thread
		VulkanAPI::SecondaryHandle secondary_cmd_buffer = cmd_buffer.begin_secondary();

		uint32_t thread_count = 0;
		for (uint32_t i = 0; i < primitives.size(); ++i) {

			if (i + 1 >= thread_group_size) {
				
				thread_pool.submitTask([&]() {

				});
			}
		}

		for (auto& prim : primitives) {

			
			cmd_buffer.secondary_bind_descriptors(mesh_pipeline.pl_layout, prim.decscriptor_set, VulkanAPI::PipelineType::Graphics, secondary_cmd_buffer);
			cmd_buffer.secondary_bind_push_block(mesh_pipeline.pl_layout, vk::ShaderStageFlagBits::eFragment, sizeof(prim.push_block), &prim.push_block, secondary_cmd_buffer);

			VkDeviceSize offset[]{ m_modelInfo[index].verticesOffset + m_vertexBuffer.offset };
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkMemory->blockBuffer(m_vertexBuffer.block_id), offset);

			// bind index data derived from face indices - draw each face with one draw call as material differ between each and we will be pushing the material data per draw call
			vkCmdBindIndexBuffer(cmdBuffer, p_vkMemory->blockBuffer(m_indexBuffer.block_id), m_indexBuffer.offset + (model.meshes[i].indexBase * sizeof(uint32_t)), VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(cmdBuffer, model.meshes[i].indexCount, 1, 0, 0, 0);
		}
	}
}