#include "SemaphoreManager.h"

#include "VulkanAPI/VkContext.h"

namespace VulkanAPI
{

SemaphoreManager::SemaphoreManager(VkContext& context) : context(context)
{
}

SemaphoreManager::~SemaphoreManager()
{
}

vk::Semaphore SemaphoreManager::getSemaphore()
{
    vk::Semaphore semaphore;

    if (!semaphores.empty())
    {
        semaphore = semaphores.back();
        semaphores.pop_back();
    }
    else
    {
        // create a new semaphore
        vk::SemaphoreCreateInfo createInfo;
        VK_CHECK_RESULT(context.device.createSemaphore(&createInfo, nullptr, &semaphore));
    }
    return semaphore;
}

void SemaphoreManager::recycle(vk::Semaphore semaphore)
{
    semaphores.push_back(semaphore);
}
} // namespace VulkanAPI
