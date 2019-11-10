#pragma once

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Common.h"

namespace VulkanAPI
{
// forward declerations
class RenderPass;
class VkContext;
class Pipeline;

using CmdBufferHandle = uint64_t;

struct CommandBufferInfo
{
	std::unique_ptr<CommandBuffer> cmdBuffer;

	// sync buffer destruction
	vk::Fence fence;

	// sync between queues
	vk::Semaphore semaphore;
};

class CmdBufferManager
{
public:

	CmdBufferManager();
	~CmdBufferManager();

	// not copyable
	CmdBufferManager(const CmdBufferManager&) = delete;
	CmdBufferManager& operator=(const CmdBufferManager) = delete;

	CmdBufferHandle newInstance();
	std::unique_ptr<CommandBuffer> &getCmdBuffer(CmdBufferHandle handle);
	std::unique_ptr<VulkanAPI::CommandBuffer> &beginNewFame(CmdBufferHandle handle);

	void submitFrame(Swapchain &swapchain);

	bool isRecorded(CmdBufferHandle handle)
	{
		return cmdBuffers[handle].cmdBuffer != nullptr;
	}

	// =============== pipeline hasher ======================
	struct PlineId
	{
		StateId() = default;

		ShaderHandle shader;
		vk::PrimitiveTopology topology;
		StateAlpha alpha;
		vk::PolygonMode polygonMode;
	};

	struct PlineHash
	{
		size_t operator()(StateId const& id) const noexcept
		{
			size_t h1 = std::hash<ShaderHandle>{}(id.shader);
			size_t h2 = std::hash<vk::PrimitiveTopology>{}(id.topology);
			size_t h3 = std::hash<StateAlpha>{}(id.alpha);
			size_t h4 = std::hash<vk::PolygonMode>{}(id.polygonMode);
			return h1 ^ (h2 << 1) ^ (h3 << 1) ^ (h4 << 1);
		}
	};

	struct PlineEqual
	{
		bool operator()(const StateId& lhs, const StateId& rhs) const
		{
			return lhs.shader == rhs.shader && lhs.topology == rhs.topology &&
			       lhs.alpha == rhs.alpha && lhs.polygonMode == rhs.polygonMode;
		}
	};

private:
	
	// The current vulkan context
	VkContext& context;

	// all the cmd buffers currently active
	std::vector<CommandBufferInfo> cmdBuffers;

	// graphic pipelines are stored in the cmd buffer for the reason that they are inextricably linked to the cmd
	// buffer during draw operations. The majority of the required data comes from the shader, but due to each pipeline being
	// exclusively tied to a renderpass, we can only create the pipeline once these have been created.
	// Note: Compute pipleines are created by the shader manager.
	std::unordered_map<PlineId, Pipeline, PlineHash, PlineEqual> states;
};
} // namespace VulkanAPI