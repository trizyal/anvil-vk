#ifndef ANVIL_VK_VULKANCONTEXT_H
#define ANVIL_VK_VULKANCONTEXT_H

#include <volk.h>
#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include "AnvilDeletionQueue.h"

class AnvilVulkanContext
{
public:
    VkInstance anvilInstance;
    VkDebugUtilsMessengerEXT anvilDebugMessenger;
    VkPhysicalDevice anvilPhysicalDevice;
    VkDevice anvilDevice;
    VkSurfaceKHR anvilSurface;

    VkQueue anvilGraphicsQueue;
    uint32_t anvilGraphicsQueueFamily;

    VmaAllocator anvilAllocator;

    AnvilDeletionQueue anvilDeletionQueue;

    void initialise(class AnvilWindow& inWindow);
    void cleanup();
};

#endif //ANVIL_VK_VULKANCONTEXT_H
