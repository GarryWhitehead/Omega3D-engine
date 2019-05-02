#pragma once
#include "RenderableBase.h"
#include "Managers/MeshManager.h"
#include "Vulkan/BufferManager.h"
#include "Vulkan/Descriptors.h"
#include "Rendering/RenderInterface.h"
#include "OEMaths/OEMaths.h"

// Number of combined image sampler sets allowed for materials. This allows for materials to be added - this value will need monitoring
#define TOTAL_MATERIAL_SETS 50

// forward decleartions
namespace VulkanAPI
{
	class Sampler;
	class CommandBuffer;
	class BufferManager;
	class VkTextureManager;
}

namespace OmegaEngine
{
	// forward declerations
	class DeferredRenderer;
	class ComponentInterface;
	class ThreadPool;
	class Object;

	// renderable object types
	class RenderableMesh : public RenderableBase
	{

	public:

		// render info that will be used to draw this mesh 
		struct MeshInstance
		{
			MeshManager::MeshType type;
			
			// pipeline
			RenderInterface::ProgramState* state = nullptr;

			// per primitive index data
			uint32_t index_primitive_offset;	// this equates to buffer_offset + sub-offset
			uint32_t index_primitive_count;
			
			// the starting offsets within the main vertices/indices buffer
			uint32_t index_offset;				// index into large buffer
			uint32_t vertex_offset;

			// vertex and index buffer memory info
			VulkanAPI::Buffer vertex_buffer;
			VulkanAPI::Buffer index_buffer;

			// all material data required to draw
			// storing this material data in two places for threading purposes.
			struct MaterialPushBlock
			{
				// colour factors
				OEMaths::vec4f baseColorFactor;
				OEMaths::vec3f emissiveFactor;
				float pad0;
				OEMaths::vec3f diffuseFactor;
				float pad1;
				OEMaths::vec3f specularFactor;
				float pad2;
				float metallicFactor;
				float roughnessFactor;

				// alpha 
				float alphaMask;
				float alphaMaskCutoff;

				// uv sets for each texture
				uint32_t baseColourUvSet;
				uint32_t metallicRoughnessUvSet;
				uint32_t normalUvSet;
				uint32_t emissiveUvSet;
				uint32_t occlusionUvSet;
			
				// indication whether we have a ceratin texture map
				uint32_t haveBaseColourMap;
				uint32_t haveNormalMap;
				uint32_t haveEmissiveMap;
				uint32_t haveMrMap;
				uint32_t haveAoMap;
				uint32_t usingSpecularGlossiness;
			} material_push_block;

			// vulkan stuff for material textures
			VulkanAPI::DescriptorSet descr_set;

			// offset into transform buffer for this mesh
			uint32_t transform_dynamic_offset = 0;
			uint32_t skinned_dynamic_offset = 0;
		};
		
		void* get_handle() override
		{
			return this;
		}

		RenderableMesh::RenderableMesh(vk::Device& device,
										std::unique_ptr<ComponentInterface>& component_manager,
										std::unique_ptr<VulkanAPI::BufferManager>& buffer_manager,
										std::unique_ptr<VulkanAPI::VkTextureManager>& texture_manager,
										MeshManager::StaticMesh mesh, 
										MeshManager::PrimitiveMesh primitive,
										Object& obj,
										RenderInterface* render_interface);

		void render(VulkanAPI::SecondaryCommandBuffer& cmd_buffer, 
					void* instance_data) override;

		static void create_mesh_pipeline(vk::Device& device,
										std::unique_ptr<RendererBase>& renderer, 
										std::unique_ptr<VulkanAPI::BufferManager>& buffer_manager,
										std::unique_ptr<VulkanAPI::VkTextureManager>& texture_manager,
										MeshManager::MeshType type,
										std::unique_ptr<RenderInterface::ProgramState>& state);

	private:


	};
}