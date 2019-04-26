#include "Vulkan/CommandBufferManager.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/Renderpass.h"

namespace VulkanAPI
{

    CommandBufferManager::CommandBufferManager(vk::Device& dev, vk::PhysicalDevice& phys_dev, Queue& queue) :
        device(dev),
        gpu(phys_dev),
        graph_queue(queue)
    {
        // set up the command buffers for swapchain presentaion now
		present_cmd_buffers.resize(swap_chain.get_image_count());
		for (uint32_t i = 0; i < present_cmd_buffers.size(); ++i) {
			present_cmd_buffers[i] = std::make_unique<CommandBuffer>(device, graph_queue.get_index());
		}
    }

    CmdBufferHandle CommandBufferManager::create_instance()
    {
        CommandBufferInfo buffer_info;
        buffer_info.cmd_buffer = std::make_unique<CommandBuffer>(device, graph_queue.get_index());

        // create a fence which will be used to sync things
		vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits::eSignaled);
        device.createFence(&fence_info, nullptr, &buffer_info.fence);

        cmd_buffers.push_back(buffer_info);
        
        return cmd_buffers.size() - 1;
    }

    std::unique_ptr<CommandBuffer>& CommandBufferManager::get_cmd_buffer(CmdBufferHandle handle)
    {
        assert(handle < cmd_buffers.size());
        return cmd_buffers[handle].cmd_buffer;
    }

    void CommandBufferManager::new_frame(CmdBufferHandle handle)
    {
        // ensure that the cmd buffer is finished with before creating a new one
        // TODO: this could slow things down if we have to wait for cmd bufefrs to finish before
        // continuing so instead have two or three buffers per handle and switch between them
        VK_CHECK_RESULT(device.waitForFences(1, &cmd_buffers[handle].fence, VK_TRUE, UINT64_MAX));
        VK_CHECK_RESULT(device.resetFences(1, &cmd_buffers[handle].fence));

        // create a new buffer - the detructors will worry about destroying everything
        // or just reset?
        cmd_buffers[handle].cmd_buffer = std::make_unique<CommandBuffer>(device, graph_queue.get_index());
    }

    void CommandBufferManager::submit_once(CmdBufferHandle handle)
    {
        
    }
    
    void CommandBufferManager::submit_frame(Swapchain& swapchain)
    {
        vk::Semaphore wait_sync;
        vk::Semaphore signal_sync;

        // begin the start of the frame by beginning the next new swapchain image
		swapchain.begin_frame(begin_semaphore);
        
        for (uint32_t i = 0; i < cmd_buffers.size(); ++i) {

            // work out the signalling and wait semaphores
            if (i == 0) {
                wait_sync = begin_semaphore;
                signal_sync = cmd_buffers[i].semaphore;
            }
            if (i == cmd_buffers.size() - 1) {
                wait_sync = cmd_buffers[i].semaphore;
                signal_sync = final_semaphore;
            }
            else {
                wait_sync = cmd_buffers[i - 1].semaphore;
                signal_sync = cmd_buffers[i].semaphore;
            }

            vk::CommandBuffer cmd_buffer = cmd_buffers[i].cmd_buffer.get();
            graph_queue.submit_cmd_buffer(cmd_buffer, wait_sync, signal_sync);

        }

        // then the presentation part.....
        swapchain.submit_frame(final_semaphore, present_queue().get());
    }

	std::unique_ptr<CommandBuffer>& CommandBufferManager::get_present_cmd_buffer(RenderPass& renderpass, uint32_t index)
	{
		auto& cmd_buffer = present_cmd_buffers[index];

		// setup the command buffer
		cmd_buffer->create_primary();

		// begin the render pass
		auto& begin_info = renderpass.get_begin_info(static_cast<vk::ClearColorValue>(render_config.general.background_col), index);
		cmd_buffer->begin_renderpass(begin_info, false);

		// set the dynamic viewport and scissor dimensions
		cmd_buffer->set_viewport();
		cmd_buffer->set_scissor();

		return cmd_buffer;
	}

}