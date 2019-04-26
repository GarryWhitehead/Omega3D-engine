#pragma once

#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Common.h"
#include "Vulkan/Queue.h"

namespace VulkanAPI
{
    enum class BufferState
    {
        Free,
        Recorded,
        Destroy
    };

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

        CommandBufferManager(vk::Device& device, vk::PhysicalDevice& phys_dev, Queue& queue);
        ~CommandBufferManager();

        std::unique_ptr<CommandBuffer>& create_cmd_buffer();

        // submits all recorded cmd buffers to the queue
        void submit_all();
        
        // submits a particular cmd buffer
        void submit(uint32_t index, bool destroy = false);

    private:

        vk::Device& device;
        vk::PhysicalDevice gpu; 
        Queue graph_queue;

        // for image and presentaion syncing 
        vk::Semaphore begin_semaphore;  // image
        vk::Semaphore final_semaphore;  // present

        // all the cmd buffers currently active
        std::vector<CommandBufferInfo> cmd_buffers;

        // presentation command bufefrs
        std::vector<CommandBuffer> present_cmd_bufefrs;
    };
}