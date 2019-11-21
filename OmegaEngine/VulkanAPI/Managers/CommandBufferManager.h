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
class VkContext;
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

	/**
	* @brief Checks whether a decsriptor set exsists and returns that if so, otherwise creates a new instance
	* The hash requires descriptor layout and pool (Vulkan handles)
	*/
    DescriptorSet* findOrCreateDescrSer(const ShaderHandle handle);
    
	/**
	* @brief Creates a new instance of a cmd buffer, including reseting fences.
	* @param queueType The queue which this cmd buffer will be submitted to.
	*/
	CmdBufferHandle newInstance(const uint32_t queueIndex);

	std::unique_ptr<VulkanAPI::CmdBuffer>& beginNewFame(CmdBufferHandle handle);

	/**
	* @brief Submits all the command buffers registered with the manager to the appropiate queue
	* Note: It might affect performance having too many cmd buffers - something that needs to be considered (maybe?)
	*/
	void submitAll(Swapchain& swapchain);

	/**
	* @brief Resets all command buffers, ensuring that they have finished before doing so
	*/
	void resetAll();

	/**
	* returns a cmdbuffer based on the specified handle 
	*/
	std::unique_ptr<CmdBuffer>& getCmdBuffer(CmdBufferHandle handle);

	bool isRecorded(CmdBufferHandle handle)
	{
		return cmdBuffers[handle].cmdBuffer != nullptr;
	}

private:
    
    /**
     * @brief Everything needed for running and syncing a command buffer
     */
    struct CmdBufferInfo
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
	VkContext& context;

	// all the cmd buffers currently active
	std::vector<CmdBufferInfo> cmdBuffers;

	// graphic pipelines are stored in the cmd buffer for the reason that they are inextricably linked to the cmd
	// buffer during draw operations. The majority of the required data comes from the shader, but due to each pipeline being
	// exclusively tied to a renderpass, we can only create the pipeline once these have been created.
	std::unordered_map<PLineHash, Pipeline, PLineHasher, PLineEqual> pipelines;
    
    // as per pipelines, descriptor sets are created and stored here
    std::unordered_map<DescrHash, DescriptorSet, DescrHasher, DescrEqual> descrSets;
};

}    // namespace VulkanAPI
