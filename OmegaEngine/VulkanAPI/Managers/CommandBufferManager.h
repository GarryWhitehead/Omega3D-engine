#pragma once

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/Descriptors.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace VulkanAPI
{
// forward declerations
class RenderPass;
class VkDriver;
class Pipeline;

using CmdBufferHandle = uint64_t;

class CmdBufferManager
{
public:
    
	CmdBufferManager(VkDriver& driver);
	~CmdBufferManager();

	// not copyable
	CmdBufferManager(const CmdBufferManager&) = delete;
	CmdBufferManager& operator=(const CmdBufferManager) = delete;

	/**
	* @brief Checks whether a piepline exsists baseed on the specified hash. Returns a pointer to the pipeline if
	* this it does, otherwise nullptr
	*/
	Pipeline* findOrCreatePipeline(const ShaderHandle handle, RenderPass* rPass);

    DescriptorSet* findOrCreateDescrSer(const ShaderHandle handle);
    
	CmdBufferHandle newInstance(const CmdBuffer::CmdBufferType type, const uint32_t queueIndex);
	std::unique_ptr<CmdBuffer>& getCmdBuffer(CmdBufferHandle handle);
	std::unique_ptr<VulkanAPI::CmdBuffer>& beginNewFame(CmdBufferHandle handle);

	void submitFrame(Swapchain& swapchain);

	bool isRecorded(CmdBufferHandle handle)
	{
		return cmdBuffers[handle].cmdBuffer != nullptr;
	}

private:
    
    /**
     * @brief Everything needed for running and syncing a command buffer
     */
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
    
	// =============== pipeline hasher ======================
	struct PLineHash
	{
		PLineHash() = default;

		ShaderHandle shader;
        RenderPass* pass;
	};

	struct PLineHasher
	{
		size_t operator()(PLineHash const& id) const noexcept
		{
			size_t h1 = std::hash<ShaderHandle>{}(id.shader);
			size_t h2 = std::hash<RenderPass*>{}(id.pass);
			return h1 ^ (h2 << 1);
		}
	};

	struct PLineEqual
	{
		bool operator()(const PLineHash& lhs, const PLineHash& rhs) const
		{
            return lhs.shader == rhs.shader && lhs.pass == rhs.pass;
		}
	};
    
    // ============== Descriptor set hasher ==================
    struct DescrHash
    {
        DescrHash() = default;

        vk::DescriptorSetLayout layout;
        vk::DescriptorPool pool;
    };

    struct DescrHasher
    {
        size_t operator()(DescrHash const& id) const noexcept
        {
            size_t h1 = std::hash<vk::DescriptorSetLayout>{}(id.layout);
            size_t h2 = std::hash<vk::DescriptorPool>{}(id.pool);
            return h1 ^ (h2 << 1);
        }
    };

    struct DescrEqual
    {
        bool operator()(const DescrHash& lhs, const DescrHash& rhs) const
        {
            return lhs.layout == rhs.layout && lhs.pool == rhs.pool;
        }
    };
    
	friend class CmdBuffer;

private:
	// The current vulkan context
	VkDriver& driver;

	// all the cmd buffers currently active
	std::vector<CommandBufferInfo> cmdBuffers;

	// graphic pipelines are stored in the cmd buffer for the reason that they are inextricably linked to the cmd
	// buffer during draw operations. The majority of the required data comes from the shader, but due to each pipeline being
	// exclusively tied to a renderpass, we can only create the pipeline once these have been created.
	std::unordered_map<PLineHash, Pipeline, PLineHasher, PLineEqual> pipelines;
    
    // as per pipelines, descriptor sets are created and stored here
    std::unordered_map<DescrHash, DescriptorSet, DescrHasher, DescrEqual> descrSets;
};

}    // namespace VulkanAPI
