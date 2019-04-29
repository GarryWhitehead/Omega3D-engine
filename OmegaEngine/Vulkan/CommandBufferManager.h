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
    }

    struct CommandBufferInfo
    {
		std::unique_ptr<CommandBuffer> cmd_buffer;

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

		CmdBufferHandle create_instance();
		std::unique_ptr<CommandBuffer>& get_cmd_buffer(CmdBufferHandle handle);
		void new_frame(CmdBufferHandle handle);

		void submit_once(CmdBufferHandle handle);
		void submit_frame(Swapchain& swapchain);

		std::unique_ptr<CommandBuffer>& begin_present_cmd_buffer(RenderPass& renderpass, vk::ClearColorValue clear_colour, uint32_t index);

		uint32_t get_present_count() const
		{
			return present_cmd_buffers.size();
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
        std::vector<CommandBufferInfo> cmd_buffers;

        // presentation command bufefrs
        std::vector<CommandBufferInfo> present_cmd_buffers;
    };
}