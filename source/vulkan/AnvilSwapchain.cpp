#include "AnvilSwapchain.h"

void AnvilSwapchain::initialise(AnvilVulkanContext& inAnvilContext, uint32_t inWidth, uint32_t inHeight)
{
    anvilVulkanContext = &inAnvilContext;

    vkb::SwapchainBuilder vkbSwapchainBuilder{
        anvilVulkanContext->anvilPhysicalDevice,
        anvilVulkanContext->anvilDevice,
        anvilVulkanContext->anvilSurface
    };

    vkbSwapchainBuilder.use_default_format_selection();
    vkbSwapchainBuilder.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR); // vsync
    vkbSwapchainBuilder.set_desired_extent(inWidth, inHeight);
    vkb::Result<vkb::Swapchain> vkbSwapchainResult = vkbSwapchainBuilder.build();

    if (!vkbSwapchainResult)
    {
        // TODO: Print detailed error message
        throw std::runtime_error("Failed to create swapchain.");
    }

    vkbSwapchain = vkbSwapchainResult.value();
    anvilImageFormat = vkbSwapchain.image_format;
    anvilImages = vkbSwapchain.get_images().value();
    anvilImageViews = vkbSwapchain.get_image_views().value();
}

void AnvilSwapchain::recreate(uint32_t inWidth, uint32_t inHeight)
{
    (void)inWidth;
    (void)inHeight;
    // TODO: Implement resizing swapchain
    throw std::logic_error("AnvilSwapchain::recreate(uint32_t inWidth, uint32_t inHeight) is not implemented yet.");
}

void AnvilSwapchain::cleanup() const
{
    for (VkImageView imageView: anvilImageViews)
    {
        vkDestroyImageView(anvilVulkanContext->anvilDevice, imageView, nullptr);
    }

    vkb::destroy_swapchain(vkbSwapchain);
}
