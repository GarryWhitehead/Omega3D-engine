#pragma once
#include "RenderableBase.h"
#include "Rendering/RenderInterface.h"
#include "Managers/MeshManager.h"
#include "Rendering/RenderableTypes/Mesh.h"

//forward declerations
namespace VulkanAPI
{
	class BufferManager;
	class SecondaryCommandBuffer;
	class Texture;
}

namespace OmegaEngine
{	
	struct ShadowComponent;
	
	class RenderableShadow : public RenderableBase
	{

	public:

		struct ShadowInstance
		{
			// pipeline
			ProgramState* state;
			
			// vertex and index buffer memory info for the cube
			VulkanAPI::Buffer vertexBuffer;
			VulkanAPI::Buffer indexBuffer;
			uint32_t indexCount = 0;

			uint32_t vertexOffset = 0;
			uint32_t indexOffset = 0;

			uint32_t transformDynamicOffset = 0;
			uint32_t skinnedDynamicOffset = 0;
			MeshManager::MeshType meshType;

			float biasConstant = 0.0f;
			float biasClamp = 0.0f;
			float biasSlope = 0.0f;
		};

		RenderableShadow(RenderInterface* renderInterface, ShadowComponent& component, RenderableMesh::MeshInstance* meshInstance);
		~RenderableShadow();
		
		static void createShadowPipeline(vk::Device& device,
										std::unique_ptr<RendererBase>& renderer, 
										std::unique_ptr<VulkanAPI::BufferManager>& bufferManager,
										std::unique_ptr<ProgramState>& state,
										MeshManager::MeshType type);
		
		static void createShadowPass(VulkanAPI::RenderPass& renderpass, 
										VulkanAPI::Texture& image,
										vk::Device& device, vk::PhysicalDevice& gpu, 
										const vk::Format format, 
										const uint32_t width, const uint32_t height);

		// used to get the address of this instance
		void* getHandle() override
		{
			return this;
		}

		void render(VulkanAPI::SecondaryCommandBuffer& cmdBuffer, 
					void* instanceData) override;
					
	private:

	};

}
