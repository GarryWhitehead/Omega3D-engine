#include "ComanndBufferManager.h"

namespace VulkanAPI
{

    CommandBufferManager::CommandBufferManager(vk::Device& dev, vk::PhysicalDevice& phys_dev, Queue& queue) :
        device(dev),
        gpu(phys_dev),
        graph_queue(queue)
    {
        // a command buffer is required for each presentation image
		swapchain_present.cmd_buffer.resize(swap_chain.get_image_count());
		for (uint32_t i = 0; i < swapchain_present.cmd_buffer.size(); ++i) {
			swapchain_present.cmd_buffer[i].init(vk_interface->get_device(), vk_interface->get_graph_queue().get_index(), VulkanAPI::CommandBuffer::UsageType::Multi);
		}
    }

    CmdBufferHandle CommandBufferManager::create_instance()
    {
        CommandBufferInfo buffer_info;
        buffer_info.cmd_buffer = std::make_unique<CommandBuffer>(device, graph_queue.get_index());

        // create a fence which will be used to sync things
        vk::FenceCreateInfo(nullptr, vk::eCreateSignalledBit);
        device.createFence(nullptr, &buffer_info.fence);

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
        VK_CHECK_RESULT(device.waitForFence(1, &cmd_buffers[handle].fence, VK_TRUE, UINT64_MAX));
        VK_CHECK_RESULT(device.resetFences(1, &cmd_buffers[handle].fence));

        // create a new buffer - the detructors will worry about destroying everything
        // or just reset?
        cmd_buffers[handle].cmd_buffer = std::make_unique<CommandBuffer>(device, graph_queue.get_index());
    }

    void CommandBufferManager::submit_once(CmdBufferHandle handle)
    {
        
    }
    
    void CommandBufferManager::submit_frame()
    {
        vk::Semaphore wait_sync;
        vk::Semaphore signal_sync;

        // begin the start of the frame by beginning the next new swapchain image
		swapchain.begin_frame(image_semaphore);
        
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
        swapchain.submit_frame(present_semaphore, vk_interface->get_present_queue().get());
    }

}