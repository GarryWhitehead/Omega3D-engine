#pragma once

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Shader.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace VulkanAPI
{
// forward declerations
class RenderPass;
class VkContext;
class Pipeline;

using CmdBufferHandle = uint64_t;

struct CommandBufferInfo
{
	std::unique_ptr<CmdBuffer> cmdBuffer;

	// sync buffer destruction
	vk::Fence fence;

	// sync between queues
	vk::Semaphore semaphore;

	// the queue to use for this cmdbuffer
	uint32_t queueIndex;

	// primary cmd pool for this buffer
	vk::CommandPool cmdPool;
};

class CmdBufferManager
{
public:
	CmdBufferManager();
	~CmdBufferManager();

	// not copyable
	CmdBufferManager(const CmdBufferManager&) = delete;
	CmdBufferManager& operator=(const CmdBufferManager) = delete;

	/**
	* @brief Checks whether a piepline exsists baseed on the specified hash. Returns a pointer to the pipeline if
	* this it does, otherwise nullptr
	*/
	Pipeline* findPipeline(const ShaderHandle handle, RenderPass* rPass);

	CmdBufferHandle newInstance(const CmdBuffer::CmdBufferType type, const uint32_t queueIndex);
	std::unique_ptr<CmdBuffer>& getCmdBuffer(CmdBufferHandle handle);
	std::unique_ptr<VulkanAPI::CmdBuffer>& beginNewFame(CmdBufferHandle handle);

	void submitFrame(Swapchain& swapchain);

	bool isRecorded(CmdBufferHandle handle)
	{
		return cmdBuffers[handle].cmdBuffer != nullptr;
	}

	// =============== pipeline hasher ======================
	struct PLineHash
	{
		PLineHash() = default;

		ShaderHandle shader;
		vk::PrimitiveTopology topology;
		StateAlpha alpha;
		vk::PolygonMode polygonMode;
	};

	struct PLineHasher
	{
		size_t operator()(PLineHash const& id) const noexcept
		{
			size_t h1 = std::hash<ShaderHandle>{}(id.shader);
			size_t h2 = std::hash<vk::PrimitiveTopology>{}(id.topology);
			size_t h3 = std::hash<StateAlpha>{}(id.alpha);
			size_t h4 = std::hash<vk::PolygonMode>{}(id.polygonMode);
			return h1 ^ (h2 << 1) ^ (h3 << 1) ^ (h4 << 1);
		}
	};

	struct PLineEqual
	{
		bool operator()(const PLineHash& lhs, const PLineHash& rhs) const
		{
			return lhs.shader == rhs.shader && lhs.topology == rhs.topology && lhs.alpha == rhs.alpha &&
			       lhs.polygonMode == rhs.polygonMode;
		}
	};

	friend class CmdBuffer;

private:
	// The current vulkan context
	VkContext& context;

	// all the cmd buffers currently active
	std::vector<CommandBufferInfo> cmdBuffers;

	// graphic pipelines are stored in the cmd buffer for the reason that they are inextricably linked to the cmd
	// buffer during draw operations. The majority of the required data comes from the shader, but due to each pipeline being
	// exclusively tied to a renderpass, we can only create the pipeline once these have been created.
	// Note: Compute pipleines are created by the shader manager.
	std::unordered_map<PLineHash, Pipeline, PLineHasher, PLineEqual> pipelines;
};
}    // namespace VulkanAPI