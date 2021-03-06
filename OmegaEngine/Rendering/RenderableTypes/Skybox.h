#pragma once
#include "RenderableBase.h"
#include "Rendering/ProgramStateManager.h"
#include "Rendering/RenderInterface.h"
#include "VulkanAPI/BufferManager.h"

//forward declerations
namespace VulkanAPI
{
class BufferManager;
class VkTextureManager;
class SecondaryCommandBuffer;
class Texture;
}    // namespace VulkanAPI

namespace OmegaEngine
{
struct SkyboxComponent;

class RenderableSkybox : public RenderableBase
{

public:
	static constexpr uint32_t indicesSize = 36;
	static constexpr uint32_t verticesSize = 24;

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

	RenderableSkybox(std::unique_ptr<ProgramStateManager>& stateManager, SkyboxComponent& component,
	                 std::unique_ptr<VulkanAPI::Interface>& vkInterface, std::unique_ptr<RendererBase>& rendderer);
	~RenderableSkybox();

	static void createSkyboxPipeline(std::unique_ptr<VulkanAPI::Interface>& vkInterface,
	                                 std::unique_ptr<RendererBase>& renderer, std::unique_ptr<ProgramState>& state,
	                                 StateId::StateFlags& flags);

	static void createSkyboxPass(VulkanAPI::RenderPass& renderpass, VulkanAPI::Texture& image,
	                             VulkanAPI::Texture& depthImage, vk::Device& device, vk::PhysicalDevice& gpu,
	                             RenderConfig& renderConfig);

	void generateBuffers();

	// used to get the address of this instance
	void* getHandle() override
	{
		return this;
	}

	void render(VulkanAPI::SecondaryCommandBuffer& cmdBuffer, void* instanceData) override;

private:
};

}    // namespace OmegaEngine
