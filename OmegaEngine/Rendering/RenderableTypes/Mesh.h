#pragma once
#include "RenderableBase.h"
#include "Managers/MeshManager.h"
#include "Vulkan/MemoryAllocator.h"
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
			
			int32_t index_offset;	// this equates to buffer_offset + sub-offset
			uint32_t index_count;
			
			// face indicies data
			uint32_t index_buffer_offset; // index into large buffer
			uint32_t index_sub_offset;	// this equates to buffer_offset + sub-offset

			uint32_t vertex_buffer_offset;

			// vertex and index buffers
			vk::Buffer vertex_buffer;
			vk::Buffer index_buffer;

			// all material data required to draw
			// storing this material data in two places for threading purposes. We could get data races
			// if we start calling back to material manager whilst is updating in a different thread
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
			VulkanAPI::Sampler sampler;

			// offset into transform buffer
			uint32_t transform_dynamic_offset = 0;
			uint32_t skinned_dynamic_offset = 0;
		};
		
		void* get_handle() override
		{
			return this;
		}

		RenderableMesh::RenderableMesh(std::unique_ptr<ComponentInterface>& component_interface, 
										MeshManager::StaticMesh mesh, 
										MeshManager::PrimitiveMesh primitive); 

		void render(VulkanAPI::CommandBuffer& cmd_buffer, 
					void* instance_data,
					RenderInterface* render_interface,
					uint32_t thread) override;

		static RenderInterface::ProgramState create_mesh_pipeline(vk::Device device,
													std::unique_ptr<RendererBase>& renderer, 
													std::unique_ptr<ComponentInterface>& component_interface,
													MeshManager::MeshType type);

	private:


	};
}