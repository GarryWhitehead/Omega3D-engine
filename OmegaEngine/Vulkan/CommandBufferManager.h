#pragma once

#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Common.h"
#include "Vulkan/Queue.h"

namespace VulkanAPI
{
	// forward declerations
	class RenderPass;
	class Swapchain;
	class SemaphoreManager;

	using CmdBufferHandle = uint64_t;

	enum class BufferState
    {
        Free,
        Recorded,
        Destroy
    };

	enum class NewFrameMode
	{
		Static,     // static scene, cmd buffer recorded once 
		Reset,      // re-use cmd buffers and reset once they have completed
		New         // create a new cmd buffer each frame
	};

    struct CommandBufferInfo
    {
		std::unique_ptr<CommandBuffer> cmdBuffer;

        // sync buffer destruction
        vk::Fence fence;

        // sync between queues
        vk::Semaphore semaphore;

        BufferState state = BufferState::Free;
	};

    class CommandBufferManager
    {
    public:

        CommandBufferManager(vk::Device& device, vk::PhysicalDevice& phys_dev, Queue& g_queue, Queue& p_queue, Swapchain& swapchain, NewFrameMode mode);
        ~CommandBufferManager();

		CmdBufferHandle createInstance();
		std::unique_ptr<CommandBuffer>& getCmdBuffer(CmdBufferHandle handle);
		void beginNewFame(CmdBufferHandle handle);

		void submit_once(CmdBufferHandle handle);
		void submitFrame(Swapchain& swapchain);

		std::unique_ptr<CommandBuffer>& beginPresentCmdBuffer(RenderPass& renderpass, vk::ClearColorValue clear_colour, uint32_t index);

		uint32_t getPresentImageCount() const
		{
			return static_cast<uint32_t>(present_cmdBuffers.size());
		}

        bool is_recorded(CmdBufferHandle handle)
        {
            return cmdBuffers[handle].cmdBuffer != nullptr;
        }

    private:

        vk::Device& device;
        vk::PhysicalDevice gpu; 
        Queue graph_queue;
		Queue present_queue;

        // States how to treat cmd buffers each frame
        NewFrameMode mode;

		// the semaphore manager is hosted here - though this may change.
		std::unique_ptr<SemaphoreManager> semaphore_manager;

        // for image and presentaion syncing 
        vk::Semaphore begin_semaphore;  // image
        vk::Semaphore final_semaphore;  // present

        // all the cmd buffers currently active
        std::vector<CommandBufferInfo> cmdBuffers;

        // presentation command bufefrs
        std::vector<CommandBufferInfo> present_cmdBuffers;
    };
}