#pragma once
#include "Managers/MeshManager.h"
#include "RenderableBase.h"
#include "Rendering/ProgramStateManager.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/RenderableTypes/Mesh.h"

//forward declerations
namespace VulkanAPI
{
class BufferManager;
class SecondaryCommandBuffer;
class Texture;
class Interface;
}    // namespace VulkanAPI

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

		uint32_t lightAlignmentSize = 0;
		uint32_t lightCount = 0;

		float biasConstant = 0.0f;
		float biasClamp = 0.0f;
		float biasSlope = 0.0f;
	};

	RenderableShadow(std::unique_ptr<ProgramStateManager>& stateManager,
	                 std::unique_ptr<VulkanAPI::Interface>& vkInterface, ShadowComponent& component, StaticMesh& mesh,
	                 PrimitiveMesh& primitive, uint32_t lightCount, uint32_t lightAlignmentSize,
	                 std::unique_ptr<RendererBase>& renderer);

	~RenderableShadow();

	static void createShadowPipeline(std::unique_ptr<VulkanAPI::Interface>& vkInterface,
	                                 std::unique_ptr<RendererBase>& renderer, std::unique_ptr<ProgramState>& state,
	                                 StateId::StateFlags& flags);

	static void createShadowPass(VulkanAPI::RenderPass& renderpass, VulkanAPI::Texture& image, vk::Device& device,
	                             vk::PhysicalDevice& gpu, const vk::Format format, const uint32_t width,
	                             const uint32_t height);

	// used to get the address of this instance
	void* getHandle() override
	{
		return this;
	}

	void render(VulkanAPI::SecondaryCommandBuffer& cmdBuffer, void* instanceData) override;

private:
};

}    // namespace OmegaEngine
