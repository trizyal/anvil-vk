// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilVulkanContext.h"

#include <stdexcept>
#include <iostream>

#include <VkBootstrap.h>

#include "AnvilWindow.h"

void AnvilVulkanContext::initializeContext(AnvilWindow& inWindow)
{
    std::cout << "Initialising AnvilVulkanContext..." << std::endl;
    // Initialise Volk
    if (volkInitialize() != VK_SUCCESS)
    {
        // TODO: Print detailed error message
        throw std::runtime_error("Failed to initialise Volk.");
    }

    // --------------------------------
    // Build Instance with vk-bootstrap
    vkb::InstanceBuilder vkbInstanceBuilder;
    vkbInstanceBuilder.set_app_name(inWindow.getWindowTitle().c_str());
#ifndef NDEBUG
    vkbInstanceBuilder.request_validation_layers(true);
    vkbInstanceBuilder.use_default_debug_messenger();

    // TODO: Add INFO and VERBOSE severities to the default messenger as a configurable option
#if 0
    vkbInstanceBuilder.add_debug_messenger_severity(
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
    );
#endif
#endif
    vkbInstanceBuilder.require_api_version(1, 4, 0);
    vkb::Result<vkb::Instance> vkbInstanceResult = vkbInstanceBuilder.build();

    if (!vkbInstanceResult)
    {
        // TODO: Print detailed error message
        throw std::runtime_error("Failed to create Vulkan instance: " + vkbInstanceResult.error().message());
    }

    const vkb::Instance vkbInstance = vkbInstanceResult.value();
    anvilInstance = vkbInstance.instance;
    anvilDebugMessenger = vkbInstance.debug_messenger;

    // --------------------------------
    // Load Instance functions into Volk
    volkLoadInstance(anvilInstance);

    // --------------------------------
    // Create Surface
    anvilSurface = inWindow.createSurface(anvilInstance);

    // --------------------------------
    // Select Physical Device
    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;

    vkb::PhysicalDeviceSelector vkbPhysicalDeviceSelector{vkbInstance};
    vkbPhysicalDeviceSelector.set_surface(anvilSurface);
    vkbPhysicalDeviceSelector.set_minimum_version(1, 3);
    vkbPhysicalDeviceSelector.add_required_extension_features(features13);
    vkb::Result<vkb::PhysicalDevice> vkbPhysicalDeviceResult = vkbPhysicalDeviceSelector.select();

    if (!vkbPhysicalDeviceResult)
    {
        // TODO: Refactor to output detailed failure reasons vector
        const auto& err = vkbPhysicalDeviceResult.detailed_failure_reasons();
        throw std::runtime_error("Failed to select physical device:\n" + err[0]);
    }

    vkb::PhysicalDevice vkbPhysicalDevice = vkbPhysicalDeviceResult.value();
    anvilPhysicalDevice = vkbPhysicalDevice.physical_device;

    // --------------------------------
    // Build Logical Device
    vkb::DeviceBuilder vkbDeviceBuilder{vkbPhysicalDevice};
    vkb::Result<vkb::Device> vkbDeviceResult = vkbDeviceBuilder.build();

    if (!vkbDeviceResult)
    {
        // TODO: Print detailed error message
        throw std::runtime_error("Failed to build device.");
    }

    vkb::Device vkbDevice = vkbDeviceResult.value();
    anvilDevice = vkbDevice.device;

    // --------------------------------
    // Load Device functions into Volk
    volkLoadDevice(anvilDevice);

    // --------------------------------
    // Get Queues
    anvilGraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    anvilGraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // --------------------------------
    // Initialise Vulkan Memory Allocator
    VmaVulkanFunctions vmaVulkanFunctions = {};
    vmaVulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vmaVulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.physicalDevice = anvilPhysicalDevice;
    allocatorCreateInfo.device = anvilDevice;
    allocatorCreateInfo.instance = anvilInstance;
    allocatorCreateInfo.pVulkanFunctions = &vmaVulkanFunctions;
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;

    if (vmaCreateAllocator(&allocatorCreateInfo, &anvilAllocator) != VK_SUCCESS)
    {
        // TODO: Print detailed error message
        throw std::runtime_error("Failed to create Vulkan Memory Allocator.");
    }

    // --------------------------------
    // Set up Deletion Queue
    anvilDeletionQueue.pushFunction([this]()
    {
        vmaDestroyAllocator(anvilAllocator);
        vkDestroyDevice(anvilDevice, nullptr);
        vkDestroySurfaceKHR(anvilInstance, anvilSurface, nullptr);
#ifndef NDEBUG
        vkb::destroy_debug_utils_messenger(anvilInstance, anvilDebugMessenger);
#endif
        vkDestroyInstance(anvilInstance, nullptr);
    });

    std::cout << "Finished initializing AnvilVulkanContext" << std::endl;
}

void AnvilVulkanContext::cleanup()
{
    anvilDeletionQueue.flush();
}
