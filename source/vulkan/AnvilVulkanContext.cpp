// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#define VOLK_IMPLEMENTATION
#include <volk.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "AnvilVulkanContext.h"

#include <stdexcept>
#include <iostream>
#include <sstream>

#include <VkBootstrap.h>

#include "AnvilVulkanDebug.h"
#include "AnvilWindow.h"

void AnvilVulkanContext::initializeVulkanContext(AnvilWindow& inWindow)
{
    std::cout << "Initialising AnvilVulkanContext..." << std::endl;

    // Initialise Volk
    if (const VkResult volk_result = volkInitialize(); volk_result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to initialise Volk. Error code: " + std::to_string(volk_result));
    }

    // --------------------------------
    // vk-bootstrap
    // --------------------------------

    // Create Instance
    vkb::InstanceBuilder vkb_instance_builder;
    vkb_instance_builder.set_app_name(inWindow.getWindowTitle().c_str());
#ifndef NDEBUG
    vkb_instance_builder.request_validation_layers(true);
    vkb_instance_builder.set_debug_callback(AnvilDebug::DebugCallback);

    // Here we enable all logs. DebugCallback handles whether to log it or not
    vkb_instance_builder.add_debug_messenger_severity(
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
    );
    vkb_instance_builder.add_debug_messenger_type(
        // VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
    );

    // TODO: implement something like vkEnumerateInstanceExtensionProperties and then get available extensions.
    // vkb_instance_builder.enable_extension(VK_EXT_DEVICE_ADDRESS_BINDING_REPORT_EXTENSION_NAME);
#endif // NDEBUG

    vkb_instance_builder.require_api_version(1, 4, 0);
    vkb::Result<vkb::Instance> vkb_instance_result = vkb_instance_builder.build();

    if (!vkb_instance_result)
    {
        std::ostringstream errorStream;
        errorStream << "Failed to create Vulkan instance via vk-bootstrap:\n"
                    << "  Primary error: " << vkb_instance_result.error().message() << "\n";
        throw std::runtime_error(errorStream.str());
    }

    const vkb::Instance vkb_instance = vkb_instance_result.value();
    anvilInstance = vkb_instance.instance;
    anvilDebugMessenger = vkb_instance.debug_messenger;

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

    // Fix for VUID-VkShaderModuleCreateInfo-pCode-08740
    VkPhysicalDeviceVulkan11Features features11{};
    features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    features11.shaderDrawParameters = VK_TRUE;

    vkb::PhysicalDeviceSelector vkbPhysicalDeviceSelector{vkb_instance};
    vkbPhysicalDeviceSelector.set_surface(anvilSurface);
    vkbPhysicalDeviceSelector.set_minimum_version(1, 3);
    vkbPhysicalDeviceSelector.add_required_extension_features(features13);
    vkbPhysicalDeviceSelector.add_required_extension_features(features11);
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
    anvilGraphicsQueueIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

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

    VkCommandPoolCreateInfo uploadPoolInfo{};
    uploadPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    uploadPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    uploadPoolInfo.queueFamilyIndex = anvilGraphicsQueueIndex;

    if (vkCreateCommandPool(anvilDevice, &uploadPoolInfo, nullptr, &uploadCommandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create upload command pool.");
    }

    VkFenceCreateInfo uploadFenceInfo{};
    uploadFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    if (vkCreateFence(anvilDevice, &uploadFenceInfo, nullptr, &uploadFence) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create upload fence.");
    }

    std::cout << "Finished initializing AnvilVulkanContext" << std::endl;
}

void AnvilVulkanContext::immediateSubmit(std::function<void(VkCommandBuffer inCmd)>&& callbackFunction)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = uploadCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(anvilDevice, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);
    callbackFunction(cmd);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(anvilGraphicsQueue, 1, &submitInfo, uploadFence);
    vkWaitForFences(anvilDevice, 1, &uploadFence, VK_TRUE, UINT64_MAX);
    vkResetFences(anvilDevice, 1, &uploadFence);

    vkFreeCommandBuffers(anvilDevice, uploadCommandPool, 1, &cmd);
}

AnvilVulkanContext::~AnvilVulkanContext()
{
    vkDestroyFence(anvilDevice, uploadFence, nullptr);
    vkDestroyCommandPool(anvilDevice, uploadCommandPool, nullptr);
    vmaDestroyAllocator(anvilAllocator);

    vkDestroyDevice(anvilDevice, nullptr);
    vkDestroySurfaceKHR(anvilInstance, anvilSurface, nullptr);
#ifndef NDEBUG
    vkb::destroy_debug_utils_messenger(anvilInstance, anvilDebugMessenger);
#endif
    vkDestroyInstance(anvilInstance, nullptr);
}
