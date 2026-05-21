#ifndef ANVIL_VK_SWAPCHAIN_H
#define ANVIL_VK_SWAPCHAIN_H

#include <vector>

#include "AnvilVulkanContext.h"

class AnvilSwapchain
{
private:
    AnvilVulkanContext *anvilVulkanContext;

public:
    vkb::Swapchain vkbSwapchain;
    VkFormat anvilImageFormat;

    std::vector<VkImage> anvilImages;
    std::vector<VkImageView> anvilImageViews;

    void initialise(AnvilVulkanContext& inAnvilContext, uint32_t inWidth, uint32_t inHeight);
    void cleanup() const;
};

#endif //ANVIL_VK_SWAPCHAIN_H
