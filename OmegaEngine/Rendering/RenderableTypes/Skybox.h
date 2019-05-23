#pragma once
#include "RenderableBase.h"
#include "Rendering/RenderInterface.h"
#include "Vulkan/BufferManager.h"

//forward declerations
namespace VulkanAPI
{
	class BufferManager;
	class VkTextureManager;
	class SecondaryCommandBuffer;
	class Texture;
}

namespace OmegaEngine
{
	struct SkyboxComponent;
	
	class RenderableSkybox : public RenderableBase
	{

	public:

		struct SkyboxInstance
		{
			// pipeline
			ProgramState* state;
			
			// vertex and index buffer memory info for the cube
			VulkanAPI::Buffer vertexBuffer;
			VulkanAPI::Buffer indexBuffer;
			uint32_t indexCount = 0;

			// the factor which the skybox will be blurred by
			float blurFactor = 0.0f;
		};

		RenderableSkybox(RenderInterface* renderInterface, SkyboxComponent& component, std::unique_ptr<VulkanAPI::BufferManager>& bufferManager);
		~RenderableSkybox();
		
		static void createSkyboxPipeline(vk::Device& device,
										std::unique_ptr<RendererBase>& renderer, 
										std::unique_ptr<VulkanAPI::BufferManager>& bufferManager,
										std::unique_ptr<VulkanAPI::VkTextureManager>& textureManager,
										std::unique_ptr<ProgramState>& state);
		
		static void createSkyboxPass(VulkanAPI::RenderPass& renderpass, VulkanAPI::Texture& image, VulkanAPI::Texture& depthImage,
			vk::Device& device, vk::PhysicalDevice& gpu, const uint32_t width, const uint32_t height);

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

