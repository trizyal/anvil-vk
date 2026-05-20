#include "AnvilVulkanContext.h"

#include <stdexcept>
#include <iostream>

#include "AnvilWindow.h"

void AnvilVulkanContext::initialise(AnvilWindow& inWindow)
{
    // Initialise Volk
    if (volkInitialize() != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to initialise Volk");
    }

    // Build Instance with vk-bootstrap
    vkb::InstanceBuilder vkbInstanceBuilder;
    vkbInstanceBuilder.set_app_name(inWindow.getWindowTitle().c_str());
    vkbInstanceBuilder.request_validation_layers(true);
    vkbInstanceBuilder.use_default_debug_messenger();
    vkbInstanceBuilder.require_api_version(1, 3, 0);
    vkb::Result<vkb::Instance> vkbInstanceResult = vkbInstanceBuilder.build();

    if (!vkbInstanceResult)
    {
        throw std::runtime_error("Failed to create Vulkan instance: " + vkbInstanceResult.error().message());
    }

    const vkb::Instance vkbInstance = vkbInstanceResult.value();
    anvilInstance = vkbInstance.instance;
    anvilDebugMessenger = vkbInstance.debug_messenger;

    // Load Instance into Volk
    volkLoadInstance(anvilInstance);
}
