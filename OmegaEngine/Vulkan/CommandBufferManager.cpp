#include "Vulkan/CommandBufferManager.h"
#include "Vulkan/SemaphoreManager.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/Renderpass.h"

namespace VulkanAPI
{

    CommandBufferManager::CommandBufferManager(vk::Device& dev, vk::PhysicalDevice& phys_dev, Queue& g_queue, Queue& p_queue, Swapchain& swapchain, NewFrameMode _mode) :
        device(dev),
        gpu(phys_dev),
        graph_queue(g_queue),
		present_queue(p_queue),
        mode(_mode)
    {
        // set up the command buffers for swapchain presentaion now
		present_cmd_buffers.resize(swapchain.get_image_count());
		for (uint32_t i = 0; i < present_cmd_buffers.size(); ++i) {

            // single use cmd buffers if static or new uffers each frame, otherwise multi-use
            if (mode == NewFrameMode::New) {
			    present_cmd_buffers[i].cmd_buffer = std::make_unique<CommandBuffer>(device, graph_queue.get_index());
            }
			else {
				// static and reset cmd buffers will be submitted multiple times 
                present_cmd_buffers[i].cmd_buffer = std::make_unique<CommandBuffer>(device, graph_queue.get_index(), CommandBuffer::UsageType::Multi);
            }

			vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits(0));
			VK_CHECK_RESULT(device.createFence(&fence_info, nullptr, &present_cmd_buffers[i].fence));
			VK_CHECK_RESULT(device.resetFences(1, &present_cmd_buffers[i].fence));
		}

		semaphore_manager = std::make_unique<SemaphoreManager>(device);
		
		// initialise semaphores required to sync frame begin and end queues
		begin_semaphore = semaphore_manager->get_semaphore();
		final_semaphore = semaphore_manager->get_semaphore();
    }

	CommandBufferManager::~CommandBufferManager()
	{
	}

    CmdBufferHandle CommandBufferManager::create_instance()
    {
        CommandBufferInfo buffer_info;
       
        // create a fence which will be used to sync things
		vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits(0));
		VK_CHECK_RESULT(device.createFence(&fence_info, nullptr, &buffer_info.fence));
		VK_CHECK_RESULT(device.resetFences(1, &buffer_info.fence)); 

		// and the semaphore used to sync between queues
		buffer_info.semaphore = semaphore_manager->get_semaphore();

		cmd_buffers.emplace_back(std::move(buffer_info));
        
        return cmd_buffers.size() - 1;
    }

    std::unique_ptr<CommandBuffer>& CommandBufferManager::get_cmd_buffer(CmdBufferHandle handle)
    {
        assert(handle < cmd_buffers.size());
        return cmd_buffers[handle].cmd_buffer;
    }

    void CommandBufferManager::new_frame(CmdBufferHandle handle)
    {
        
        // if it's static then only create a new instance if it's null
        if (mode == NewFrameMode::Static) {
            if (cmd_buffers[handle].cmd_buffer == nullptr) {
                cmd_buffers[handle].cmd_buffer = std::make_unique<CommandBuffer>(device, graph_queue.get_index(), CommandBuffer::UsageType::Multi);
            }
			else {
				VK_CHECK_RESULT(device.waitForFences(1, &cmd_buffers[handle].fence, VK_TRUE, UINT64_MAX));
				VK_CHECK_RESULT(device.resetFences(1, &cmd_buffers[handle].fence));
			}
        }
		else {
			// ensure that the cmd buffer is finished with before creating a new one
			// TODO: this could slow things down if we have to wait for cmd bufefrs to finish before
			// continuing so instead have two or three buffers per handle and switch between them
			VK_CHECK_RESULT(device.waitForFences(1, &cmd_buffers[handle].fence, VK_TRUE, UINT64_MAX));
			VK_CHECK_RESULT(device.resetFences(1, &cmd_buffers[handle].fence));

			// create a new buffer - the detructors will worry about destroying everything
			// or just reset?
			if (mode == NewFrameMode::New) {
				cmd_buffers[handle].cmd_buffer = std::make_unique<CommandBuffer>(device, graph_queue.get_index());
			}
			else {
				if (cmd_buffers[handle].cmd_buffer == nullptr) {
					cmd_buffers[handle].cmd_buffer = std::make_unique<CommandBuffer>(device, graph_queue.get_index(), CommandBuffer::UsageType::Multi);
				}
				else {
					cmd_buffers[handle].cmd_buffer.reset({});
				}
			}
		}
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

		uint32_t frame_index = swapchain.get_image_index();

        for (uint32_t i = 0; i <= cmd_buffers.size(); ++i) {

			vk::CommandBuffer cmd_buffer;
			vk::Fence fence;

            // work out the signalling and wait semaphores
            if (i == 0) {
                wait_sync = begin_semaphore;
                signal_sync = cmd_buffers[i].semaphore;
				cmd_buffer = cmd_buffers[i].cmd_buffer->get();
				fence = cmd_buffers[i].fence;
            }
            else if (i == cmd_buffers.size()) {
                wait_sync = cmd_buffers[i - 1].semaphore;
                signal_sync = final_semaphore;
				cmd_buffer = present_cmd_buffers[frame_index].cmd_buffer->get();
				fence = present_cmd_buffers[frame_index].fence;
            }
            else {
                wait_sync = cmd_buffers[i - 1].semaphore;
                signal_sync = cmd_buffers[i].semaphore;
				cmd_buffer = cmd_buffers[i].cmd_buffer->get();
				fence = cmd_buffers[i].fence;
            }

			VK_CHECK_RESULT(device.resetFences(1, &fence));
            graph_queue.submit_cmd_buffer(cmd_buffer, wait_sync, signal_sync, fence);
        }

        // then the presentation part.....
        swapchain.submit_frame(final_semaphore, present_queue.get());
    }

	std::unique_ptr<CommandBuffer>& CommandBufferManager::begin_present_cmd_buffer(RenderPass& renderpass, vk::ClearColorValue clear_colour, uint32_t index)
	{
		auto& cmd_buffer = present_cmd_buffers[index].cmd_buffer;

		// setup the command buffer
		cmd_buffer->create_primary();

		// begin the render pass
		auto& begin_info = renderpass.get_begin_info(clear_colour, index);
		cmd_buffer->begin_renderpass(begin_info, false);

		// set the dynamic viewport and scissor dimensions
		cmd_buffer->set_viewport();
		cmd_buffer->set_scissor();

		return cmd_buffer;
	}

}