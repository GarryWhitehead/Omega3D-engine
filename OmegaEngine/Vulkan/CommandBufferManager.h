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

        CommandBufferManager(vk::Device& device, vk::PhysicalDevice& physicalDevice, Queue& g_queue, Queue& p_queue, Swapchain& swapchain, NewFrameMode mode);
        ~CommandBufferManager();

		CmdBufferHandle createInstance();
		std::unique_ptr<CommandBuffer>& getCmdBuffer(CmdBufferHandle handle);
		std::unique_ptr<VulkanAPI::CommandBuffer>&  beginNewFame(CmdBufferHandle handle);

		void submitOnce(CmdBufferHandle handle);
		void submitFrame(Swapchain& swapchain);

		std::unique_ptr<CommandBuffer>& beginPresentCmdBuffer(RenderPass& renderpass, vk::ClearColorValue clear_colour, uint32_t index);

		uint32_t getPresentImageCount() const
		{
			return static_cast<uint32_t>(presentionCmdBuffers.size());
		}

        bool isRecorded(CmdBufferHandle handle)
        {
            return cmdBuffers[handle].cmdBuffer != nullptr;
        }

    private:

        vk::Device& device;
        vk::PhysicalDevice gpu; 
        Queue graphicsQueue;
		Queue presentionQueue;

        // States how to treat cmd buffers each frame
        NewFrameMode mode;

		// the semaphore manager is hosted here - though this may change.
		std::unique_ptr<SemaphoreManager> semaphoreManager;

        // for image and presentaion syncing 
        vk::Semaphore beginSemaphore;  // image
        vk::Semaphore finalSemaphore;  // present

        // all the cmd buffers currently active
        std::vector<CommandBufferInfo> cmdBuffers;

        // presentation command bufefrs
        std::vector<CommandBufferInfo> presentionCmdBuffers;
    };
}