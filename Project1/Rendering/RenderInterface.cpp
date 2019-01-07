#include "RenderInterface.h"
#include "ComponentInterface/ComponentInterface.h"
#include "DataTypes/Object.h"
#include "Managers/TransformManager.h"
#include "Managers/MeshManager.h"
#include "Managers/MaterialManager.h"
#include "Managers/TextureManager.h"
#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Utility/logger.h"

namespace OmegaEngine
{

	RenderInterface::RenderInterface(VulkanAPI::Device device, const uint32_t win_width, const uint32_t win_height)
	{
		// initiliase the graphical backend - we are solely using Vulkan 
		vk_interface = std::make_unique<VulkanAPI::Interface>(device, win_width, win_height);

		// initlaise all shaders that will be used which is dependent on the number of renderable types
		for (uint16_t r_type = 0; r_type < (uint16_t)RenderTypes::Count; ++r_type) {
			this->add_shader((RenderTypes)r_type);
		}
	}


	RenderInterface::~RenderInterface()
	{
	}

	void RenderInterface::add_static_mesh(std::unique_ptr<ComponentInterface>& comp_interface, Object& obj)
	{
		auto &transform_man = comp_interface->getManager<TransformManager>();
		auto &mesh_man = comp_interface->getManager<MeshManager>();

		MeshManager::StaticMesh mesh = mesh_man->getStaticMesh(obj);

		// upload mesh data to GPU
		RenderableMesh r_mesh;

		assert(!mesh.vertexBuffer.empty());
		assert(!mesh.indexBuffer.empty());
		vk_interface->get_mem_alloc()->mapDataToSegment<MeshManager::Vertex>(r_mesh.vertices, mesh.vertexBuffer);
		vk_interface->get_mem_alloc()->mapDataToSegment<uint32_t>(r_mesh.indicies, mesh.indexBuffer);

		// sort index offsets and materials ready for rendering
		for (auto& prim : mesh.primitives) {

			RenderableMesh::PrimitiveMesh r_prim;

			VulkanAPI::Texture tex(VulkanAPI::TextureType::Normal);
			auto& mat = comp_interface->getManager<MaterialManager>()->get(prim.materialId);

			// set up the push block 
			r_prim.push_block.create(mat);

			// map all of the pbr materials for this primitive mesh to the gpu
			for (uint8_t i = 0; i < (uint8_t)PbrMaterials::Count; ++i) {
				tex.map(comp_interface->getManager<TextureManager>()->get_texture(mat.textures[i]), vk_interface->get_mem_alloc());

				// get the image views for each texture as we will need this for the descriptors
				r_prim.image_views[i] = tex.get_image_view();

				// indices data which will be used for creating the cmd buffers
				r_prim.index_offset = prim.indexBase;
				r_prim.index_count = prim.indexCount;
			}
			
		}
	} 

	void RenderInterface::add_shader(RenderTypes type)
	{
		VulkanAPI::Shader shader;
		switch (type) {
		case OmegaEngine::RenderTypes::Mesh:
			shader.add(device, "models/model.vert", VulkanAPI::StageType::Vertex, "models/model.frag", VulkanAPI::StageType::Fragment);
			break;
		case OmegaEngine::RenderTypes::Skybox:
			shader.add(device, "env/skybox.vert", VulkanAPI::StageType::Vertex, "env/skybox.frag", VulkanAPI::StageType::Fragment);
			break;
		default:
			LOGGER_INFO("Unsupported render type found whilst initilaising shaders.");
		}

		pipelines[(int)type].shader = shader;
	}

}
