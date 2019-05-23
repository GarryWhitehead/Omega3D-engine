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
		present_cmdBuffers.resize(swapchain.getImageCount());
		for (uint32_t i = 0; i < present_cmdBuffers.size(); ++i) {

            // single use cmd buffers if static or new uffers each frame, otherwise multi-use
            if (mode == NewFrameMode::New) {
			    present_cmdBuffers[i].cmdBuffer = std::make_unique<CommandBuffer>(device, graph_queue.get_index());
            }
			else {
				// static and reset cmd buffers will be submitted multiple times 
                present_cmdBuffers[i].cmdBuffer = std::make_unique<CommandBuffer>(device, graph_queue.get_index(), CommandBuffer::UsageType::Multi);
            }

			vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits(0));
			VK_CHECK_RESULT(device.createFence(&fence_info, nullptr, &present_cmdBuffers[i].fence));
			VK_CHECK_RESULT(device.resetFences(1, &present_cmdBuffers[i].fence));
		}

		semaphore_manager = std::make_unique<SemaphoreManager>(device);
		
		// initialise semaphores required to sync frame begin and end queues
		begin_semaphore = semaphore_manager->get_semaphore();
		final_semaphore = semaphore_manager->get_semaphore();
    }

	CommandBufferManager::~CommandBufferManager()
	{
	}

    CmdBufferHandle CommandBufferManager::createInstance()
    {
        CommandBufferInfo buffer_info;
       
        // create a fence which will be used to sync things
		vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits(0));
		VK_CHECK_RESULT(device.createFence(&fence_info, nullptr, &buffer_info.fence));
		VK_CHECK_RESULT(device.resetFences(1, &buffer_info.fence)); 

		// and the semaphore used to sync between queues
		buffer_info.semaphore = semaphore_manager->get_semaphore();

		cmdBuffers.emplace_back(std::move(buffer_info));
        
        return cmdBuffers.size() - 1;
    }

    std::unique_ptr<CommandBuffer>& CommandBufferManager::getCmdBuffer(CmdBufferHandle handle)
    {
        assert(handle < cmdBuffers.size());
        return cmdBuffers[handle].cmdBuffer;
    }

    void CommandBufferManager::beginNewFame(CmdBufferHandle handle)
    {
        
        // if it's static then only create a new instance if it's null
        if (mode == NewFrameMode::Static) {
            if (cmdBuffers[handle].cmdBuffer == nullptr) {
                cmdBuffers[handle].cmdBuffer = std::make_unique<CommandBuffer>(device, graph_queue.get_index(), CommandBuffer::UsageType::Multi);
            }
			else {
				VK_CHECK_RESULT(device.waitForFences(1, &cmdBuffers[handle].fence, VK_TRUE, UINT64_MAX));
				VK_CHECK_RESULT(device.resetFences(1, &cmdBuffers[handle].fence));
			}
        }
		else {
			// ensure that the cmd buffer is finished with before creating a new one
			// TODO: this could slow things down if we have to wait for cmd bufefrs to finish before
			// continuing so instead have two or three buffers per handle and switch between them
			VK_CHECK_RESULT(device.waitForFences(1, &cmdBuffers[handle].fence, VK_TRUE, UINT64_MAX));
			VK_CHECK_RESULT(device.resetFences(1, &cmdBuffers[handle].fence));

			// create a new buffer - the detructors will worry about destroying everything
			// or just reset?
			if (mode == NewFrameMode::New) {
				cmdBuffers[handle].cmdBuffer = std::make_unique<CommandBuffer>(device, graph_queue.get_index());
			}
			else {
				if (cmdBuffers[handle].cmdBuffer == nullptr) {
					cmdBuffers[handle].cmdBuffer = std::make_unique<CommandBuffer>(device, graph_queue.get_index(), CommandBuffer::UsageType::Multi);
				}
				else {
					cmdBuffers[handle].cmdBuffer.reset({});
				}
			}
		}
    }

    void CommandBufferManager::submit_once(CmdBufferHandle handle)
    {
        
    }
    
    void CommandBufferManager::submitFrame(Swapchain& swapchain)
    {
        vk::Semaphore wait_sync;
        vk::Semaphore signal_sync;

        // begin the start of the frame by beginning the next new swapchain image
		swapchain.begin_frame(begin_semaphore);

		uint32_t frame_index = swapchain.getImage_index();

        for (uint32_t i = 0; i <= cmdBuffers.size(); ++i) {

			vk::CommandBuffer cmdBuffer;
			vk::Fence fence;

            // work out the signalling and wait semaphores
            if (i == 0) {
                wait_sync = begin_semaphore;
                signal_sync = cmdBuffers[i].semaphore;
				cmdBuffer = cmdBuffers[i].cmdBuffer->get();
				fence = cmdBuffers[i].fence;
            }
            else if (i == cmdBuffers.size()) {
                wait_sync = cmdBuffers[i - 1].semaphore;
                signal_sync = final_semaphore;
				cmdBuffer = present_cmdBuffers[frame_index].cmdBuffer->get();
				fence = present_cmdBuffers[frame_index].fence;
            }
            else {
                wait_sync = cmdBuffers[i - 1].semaphore;
                signal_sync = cmdBuffers[i].semaphore;
				cmdBuffer = cmdBuffers[i].cmdBuffer->get();
				fence = cmdBuffers[i].fence;
            }

			VK_CHECK_RESULT(device.resetFences(1, &fence));
            graph_queue.submit_cmdBuffer(cmdBuffer, wait_sync, signal_sync, fence);
        }

        // then the presentation part.....
        swapchain.submitFrame(final_semaphore, present_queue.get());
    }

	std::unique_ptr<CommandBuffer>& CommandBufferManager::beginPresentCmdBuffer(RenderPass& renderpass, vk::ClearColorValue clear_colour, uint32_t index)
	{
		auto& cmdBuffer = present_cmdBuffers[index].cmdBuffer;

		// setup the command buffer
		cmdBuffer->createPrimary();

		// begin the render pass
		auto& beginInfo = renderpass.getBeginInfo(clear_colour, index);
		cmdBuffer->beginRenderpass(beginInfo, false);

		return cmdBuffer;
	}

}