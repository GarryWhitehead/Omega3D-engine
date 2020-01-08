#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/CommandBuffer.h"

#include "VulkanAPI/SemaphoreManager.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

#define MAX_FRAMES_IN_FLIGHT 3

namespace VulkanAPI
{
// forward declerations
class RenderPass;
class VkContext;
class Pipeline;
class CmdBuffer;
class ShaderProgram;
class FrameBuffer;
class Swapchain;

using CmdBufferHandle = uint64_t;

/**
 * @brief A pool of command buffers and everything required for syncronisation
 */
class CmdPool
{
public:
    
    /// Defines how many signle use command buffers to create when initialising
    static constexpr uint8_t SingleUseCbSize = 5;
    
	CmdPool(VkContext& context, SemaphoreManager& spManager, uint32_t queueIndex);
	~CmdPool();

	/**
	* @brief Creates a new instance of a cmd buffer, including reseting fences.
	* Returns a pointer to the buffer - because we never destroy buffers, only reset them, its safe to pass around pointers.
	*/
	CmdBuffer* createPrimaryCmdBuffer(CmdBufferManager* manager);
    
    CmdBuffer* createSecCmdBuffer(CmdBufferManager* manager);
    
    /**
     * @brief Either returns a cmd buffer from the free pool or creates a new instance. Cmd buffers should be released back to the pool after use by calling
     * **releaseSingleUseCb**
     */
    std::unique_ptr<CmdBuffer> getSingleUseCb(CmdBufferManager* manager);
    
    void releaseSingleUseCb(std::unique_ptr<CmdBuffer> cmdBuffer);
    
    /**
    * @brief Submits all the command buffers registered with the manager to the appropiate queue
    * Note: It might affect performance having too many cmd buffers - something that needs to be considered (maybe?)
    */
    void submitAll(Swapchain& swapchain, const uint32_t imageIndex, const vk::Semaphore& beginSemaphore);
    
	/**
	* @brief Resets the command pool. Before doing this, all cmd buffers are checked to make sure they have finished their activites.
	*/
	void reset();
    
    void clearSecondary();
    
    std::vector<vk::CommandBuffer> getSecondary();
    
    vk::CommandPool& get()
    {
        return cmdPool;
    }
    
private:
	struct CmdInstance
	{
		std::unique_ptr<CmdBuffer> cmdBuffer;

		/// to ensure the cmd buffer has finished before resetting
		vk::Fence fence;

		/// sync between queues
		vk::Semaphore semaphore;
	};

	VkContext& context;
	SemaphoreManager& spManager;

	/// all the cmd buffers that have been created with this pool
	std::vector<CmdInstance> cmdInstances;
    
    /// secondary command buffers which are associated with this pool
    std::vector<std::unique_ptr<CmdBuffer>> secondary;
    
    /// single use cmd buffers that are recycled after use
    std::vector<std::unique_ptr<CmdBuffer>> singleUseCbs;

	/// the queue to use for this pool
	uint32_t queueIndex;

	vk::CommandPool cmdPool;
};

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

	CmdPool* createMainPool();
    
    std::unique_ptr<CmdPool> createSecondaryPool();
    
    void releasePool(std::unique_ptr<CmdPool> pool);
    
	void beginNewFame();
    
    void submitFrame(Swapchain& swapchain, const uint32_t imageIndex, const vk::Semaphore& beginSemaphore);
    
	// =============== renderpass functions ================================

	void beginRenderpass(CmdBuffer* cmdBuffer, RenderPass& rpass, FrameBuffer& fbuffer);

	void endRenderpass(CmdBuffer* cmdBuffer);

    // =============== getters ==============================================
    
    CmdPool* getMainPool()
    {
        return mainPool.get();
    }
    
    std::unique_ptr<CmdBuffer> getSingleUseCb();
    
    void releaseSingleUseCb(std::unique_ptr<CmdBuffer> cmdBuffer);
    
private:
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
			size_t h1 = std::hash<ShaderProgram*>{}(id.prog);
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

	friend class CmdBuffer;

private:
	// The current vulkan context
	VkContext& context;

	// the main cmd pool used for the main thread
	std::unique_ptr<CmdPool> mainPool;

    // command pools used for threading
    std::vector<std::unique_ptr<CmdPool>> secondaryPools;
    
	// graphic pipelines are stored in the cmd buffer for the reason that they are inextricably linked to the cmd
	// buffer during draw operations. The majority of the required data comes from the shader, but due to each pipeline being
	// exclusively tied to a renderpass, we can only create the pipeline once these have been created.
	std::unordered_map<PLineHash, Pipeline*, PLineHasher, PLineEqual> pipelines;

	std::unique_ptr<SemaphoreManager> spManager;

	// the current frame which to draw
	uint8_t currFrame = 0;
};

}    // namespace VulkanAPI
