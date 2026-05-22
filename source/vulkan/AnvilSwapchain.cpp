#include "AnvilSwapchain.h"

void AnvilSwapchain::initialise(AnvilVulkanContext& inAnvilContext, uint32_t inWidth, uint32_t inHeight)
{
    anvilVulkanContext = &inAnvilContext;
    buildSwapchainInternal(inWidth, inHeight);
}

void AnvilSwapchain::recreate(const uint32_t inWidth, const uint32_t inHeight)
{
    // Wait for GPU to finish
    vkDeviceWaitIdle(anvilVulkanContext->anvilDevice);
    cleanup();

    buildSwapchainInternal(inWidth, inHeight);
}

void AnvilSwapchain::cleanup()
{
    for (VkImageView imageView: anvilImageViews)
    {
        vkDestroyImageView(anvilVulkanContext->anvilDevice, imageView, nullptr);
    }

    vkDestroySwapchainKHR(anvilVulkanContext->anvilDevice, anvilSwapchain, nullptr);
    anvilSwapchain = VK_NULL_HANDLE;
}

void AnvilSwapchain::buildSwapchainInternal(const uint32_t inWidth, const uint32_t inHeight)
{
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

    vkb::Swapchain vkbSwapchain = vkbSwapchainResult.value();
    anvilSwapchain = vkbSwapchain.swapchain;
    anvilExtent = vkbSwapchain.extent;
    anvilImageFormat = vkbSwapchain.image_format;

    anvilImages = vkbSwapchain.get_images().value();
    anvilImageViews = vkbSwapchain.get_image_views().value();
}
