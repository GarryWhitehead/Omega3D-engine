#pragma once
#include "RenderableBase.h"
#include "Managers/MeshManager.h"
#include "VulkanAPI/BufferManager.h"
#include "VulkanAPI/Descriptors.h"
#include "Rendering/RenderInterface.h"
#include "OEMaths/OEMaths.h"

// Number of combined image sampler sets allowed for materials. This allows for materials to be added - this value will need monitoring
#define MAX_MATERIAL_SETS 50

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
			ProgramState* state = nullptr;

			// per primitive index data
			uint32_t indexPrimitiveOffset;	// this equates to buffer_offset + sub-offset
			uint32_t indexPrimitiveCount;
			
			// the starting offsets within the main vertices/indices buffer
			uint32_t indexOffset;				// index into large buffer
			uint32_t vertexOffset;

			// vertex and index buffer memory info
			VulkanAPI::Buffer vertexBuffer;
			VulkanAPI::Buffer indexBuffer;

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
			} materialPushBlock;

			// vulkan stuff for material textures
			VulkanAPI::DescriptorSet descriptorSet;

			// offset into transform buffer for this mesh
			uint32_t transformDynamicOffset = 0;
			uint32_t skinnedDynamicOffset = 0;
		};
		
		void* getHandle() override
		{
			return this;
		}

		RenderableMesh::RenderableMesh(vk::Device& device,
										std::unique_ptr<ComponentInterface>& componentInterface,
										std::unique_ptr<VulkanAPI::BufferManager>& bufferManager,
										std::unique_ptr<VulkanAPI::VkTextureManager>& textureManager,
										MeshManager::StaticMesh& mesh, 
										MeshManager::PrimitiveMesh& primitive,
										Object& obj,
										RenderInterface* renderInterface);

		void render(VulkanAPI::SecondaryCommandBuffer& cmdBuffer, 
					void* instanceData) override;

		static void createMeshPipeline(vk::Device& device,
										std::unique_ptr<RendererBase>& renderer, 
										std::unique_ptr<VulkanAPI::BufferManager>& bufferManager,
										std::unique_ptr<VulkanAPI::VkTextureManager>& textureManager,
										MeshManager::MeshType type,
										std::unique_ptr<ProgramState>& state);

	private:


	};
}