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
class CmdBuffer;
class ShaderProgram;

using CmdBufferHandle = uint64_t;

class CmdBufferManager
{
public:
    
	CmdBufferManager(VkContext& context);
	~CmdBufferManager();

	// not copyable
	CmdBufferManager(const CmdBufferManager&) = delete;
	CmdBufferManager& operator=(const CmdBufferManager) = delete;

	/**
	* @brief Checks whether a piepline exsists baseed on the specified hash. Returns a pointer to the pipeline if
	* it does, otherwise nullptr
	*/
	Pipeline* findOrCreatePipeline(ShaderProgram* prog, RenderPass* rPass);

	/**
	* @brief Checks whether a decsriptor set exsists and returns that if so, otherwise creates a new instance
	* The hash requires descriptor layout and pool (Vulkan handles)
	*/
    DescriptorSet* findOrCreateDescrSet(DescriptorLayout& layout);
    
	/**
	* @brief Creates a new instance of a cmd buffer, including reseting fences.
	* @param queueType The queue which this cmd buffer will be submitted to.
	*/
	CmdBufferHandle createCmdBuffer();

	std::unique_ptr<CmdBuffer>& beginNewFame(CmdBufferHandle handle);

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


	// =============== renderpass functions ================================

	void beginRenderpass(const CmdBufferHandle handle, RenderPass& rpass, FrameBuffer& fbuffer);

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

        /// sync buffer destruction
        vk::Fence fence;

        /// sync between queues
        vk::Semaphore semaphore;

        /// the queue to use for this cmdbuffer
        uint32_t queueIndex;

        /// primary cmd pool for this buffer
        vk::CommandPool cmdPool;
    };
    
	// =============== pipeline hasher ======================
	struct PLineHash
	{
		// first three comprise the shader hash
		ShaderProgram* prog;
        RenderPass* pass;
	};

	struct PLineHasher
	{
		size_t operator()(PLineHash const& id) const noexcept
		{
			size_t h1 = std::hash<const char*>{}(id.prog);
			size_t h2 = std::hash<RenderPass*>{}(id.pass);
			return h1 ^ (h2 << 1);
		}
	};

	struct PLineEqual
	{
		bool operator()(const PLineHash& lhs, const PLineHash& rhs) const
		{
            return lhs.prog == rhs.prog && lhs.pass == rhs.pass;
		}
	};
    
    // ============== Descriptor set hasher ==================
    // using the address of the descriptor layout as this will be uniuqe
    struct DescrHash
    {
        vk::DescriptorLayout* layout = nullptr;
    };

    struct DescrHasher
    {
        size_t operator()(DescrHash const& id) const noexcept
        {
            return std::hash<vDescriptorLayout*>{}(id.layout);
        }
    };

    struct DescrEqual
    {
        bool operator()(const DescrHash& lhs, const DescrHash& rhs) const
        {
            return lhs.layout == rhs.layout;
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
