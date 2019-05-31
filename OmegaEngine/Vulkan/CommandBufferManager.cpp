#include "Vulkan/CommandBufferManager.h"
#include "Vulkan/SemaphoreManager.h"
#include "Vulkan/SwapChain.h"
#include "Vulkan/Renderpass.h"

namespace VulkanAPI
{

    CommandBufferManager::CommandBufferManager(vk::Device& dev, vk::PhysicalDevice& physicalDevice, Queue& g_queue, Queue& p_queue, Swapchain& swapchain, NewFrameMode _mode) :
        device(dev),
        gpu(physicalDevice),
        graphicsQueue(g_queue),
		presentionQueue(p_queue),
        mode(_mode)
    {
        // set up the command buffers for swapchain presentaion now
		presentionCmdBuffers.resize(swapchain.getImageCount());
		for (uint32_t i = 0; i < presentionCmdBuffers.size(); ++i) {

            // single use cmd buffers if static or new uffers each frame, otherwise multi-use
            if (mode == NewFrameMode::New) {
			    presentionCmdBuffers[i].cmdBuffer = std::make_unique<CommandBuffer>(device, graphicsQueue.getIndex());
            }
			else {
				// static and reset cmd buffers will be submitted multiple times 
                presentionCmdBuffers[i].cmdBuffer = std::make_unique<CommandBuffer>(device, graphicsQueue.getIndex(), CommandBuffer::UsageType::Multi);
            }

			vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits(0));
			VK_CHECK_RESULT(device.createFence(&fence_info, nullptr, &presentionCmdBuffers[i].fence));
			VK_CHECK_RESULT(device.resetFences(1, &presentionCmdBuffers[i].fence));
		}

		semaphoreManager = std::make_unique<SemaphoreManager>(device);
		
		// initialise semaphores required to sync frame begin and end queues
		beginSemaphore = semaphoreManager->getSemaphore();
		finalSemaphore = semaphoreManager->getSemaphore();
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
		buffer_info.semaphore = semaphoreManager->getSemaphore();

		cmdBuffers.emplace_back(std::move(buffer_info));
        
        return cmdBuffers.size() - 1;
    }

    std::unique_ptr<CommandBuffer>& CommandBufferManager::getCmdBuffer(CmdBufferHandle handle)
    {
        assert(handle < cmdBuffers.size());
        return cmdBuffers[handle].cmdBuffer;
    }

    std::unique_ptr<VulkanAPI::CommandBuffer>& CommandBufferManager::beginNewFame(CmdBufferHandle handle)
    {
        
        // if it's static then only create a new instance if it's null
        if (mode == NewFrameMode::Static) {
            if (cmdBuffers[handle].cmdBuffer == nullptr) {
                cmdBuffers[handle].cmdBuffer = std::make_unique<CommandBuffer>(device, graphicsQueue.getIndex(), CommandBuffer::UsageType::Multi);
				cmdBuffers[handle].cmdBuffer->createPrimary();
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
				cmdBuffers[handle].cmdBuffer = std::make_unique<CommandBuffer>(device, graphicsQueue.getIndex());
			}
			else {
				if (cmdBuffers[handle].cmdBuffer == nullptr) {
					cmdBuffers[handle].cmdBuffer = std::make_unique<CommandBuffer>(device, graphicsQueue.getIndex(), CommandBuffer::UsageType::Multi);
					cmdBuffers[handle].cmdBuffer->createPrimary();
				}
				else {
					cmdBuffers[handle].cmdBuffer.reset({});
				}
			}
		}

		return cmdBuffers[handle].cmdBuffer;
    }

    void CommandBufferManager::submitOnce(CmdBufferHandle handle)
    {
        
    }
    
    void CommandBufferManager::submitFrame(Swapchain& swapchain)
    {
        vk::Semaphore waitSync;
        vk::Semaphore signalSync;

        // begin the start of the frame by beginning the next new swapchain image
		swapchain.begin_frame(beginSemaphore);

		uint32_t frameIndex = swapchain.getImageIndex();

        for (uint32_t i = 0; i <= cmdBuffers.size(); ++i) {

			vk::CommandBuffer cmdBuffer;
			vk::Fence fence;

            // work out the signalling and wait semaphores
            if (i == 0) {
                waitSync = beginSemaphore;
                signalSync = cmdBuffers[i].semaphore;
				cmdBuffer = cmdBuffers[i].cmdBuffer->get();
				fence = cmdBuffers[i].fence;
            }
            else if (i == cmdBuffers.size()) {
                waitSync = cmdBuffers[i - 1].semaphore;
                signalSync = finalSemaphore;
				cmdBuffer = presentionCmdBuffers[frameIndex].cmdBuffer->get();
				fence = presentionCmdBuffers[frameIndex].fence;
            }
            else {
                waitSync = cmdBuffers[i - 1].semaphore;
                signalSync = cmdBuffers[i].semaphore;
				cmdBuffer = cmdBuffers[i].cmdBuffer->get();
				fence = cmdBuffers[i].fence;
            }

			VK_CHECK_RESULT(device.resetFences(1, &fence));
            graphicsQueue.submitCmdBuffer(cmdBuffer, waitSync, signalSync, fence);
        }

        // then the presentation part.....
        swapchain.submitFrame(finalSemaphore, presentionQueue.get());
    }

	std::unique_ptr<CommandBuffer>& CommandBufferManager::beginPresentCmdBuffer(RenderPass& renderpass, vk::ClearColorValue clear_colour, uint32_t index)
	{
		auto& cmdBuffer = presentionCmdBuffers[index].cmdBuffer;

		// setup the command buffer
		cmdBuffer->createPrimary();

		// begin the render pass
		auto& beginInfo = renderpass.getBeginInfo(clear_colour, index);
		cmdBuffer->beginRenderpass(beginInfo, false);

		return cmdBuffer;
	}

}