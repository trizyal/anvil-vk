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
#include "AnvilVulkanResult.h"
#include "AnvilWindow.h"

void AnvilVulkanContext::initializeVulkanContext(AnvilWindow& inWindow)
{
    std::cout << "Initialising AnvilVulkanContext..." << std::endl;

    // --------------------------------
    // Initialise Volk
    if (const VkResult volk_result = volkInitialize(); volk_result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to initialise Volk. Error code: " + std::to_string(volk_result));
    }

    // --------------------------------
    // Create Instance
    vkb::InstanceBuilder vkb_instance_builder;
    vkb_instance_builder.set_app_name(inWindow.getWindowTitle().c_str());
    vkb_instance_builder.require_api_version(1, 4, 0);

#   ifndef NDEBUG
    vkb_instance_builder.request_validation_layers(true);
    vkb_instance_builder.set_debug_callback(AnvilDebug::DebugCallback);

    // Here we enable all severities. DebugCallback handles whether to log it or not
    vkb_instance_builder.add_debug_messenger_severity(
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
    );

    // Here we enable all messages. DebugCallback handles whether to log it or not
    vkb_instance_builder.add_debug_messenger_type(
        // VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
    );

    // TODO: implement something like vkEnumerateInstanceExtensionProperties and then get available extensions.
    // vkb_instance_builder.enable_extension(VK_EXT_DEVICE_ADDRESS_BINDING_REPORT_EXTENSION_NAME);
#   endif // NDEBUG

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

    vkb::PhysicalDeviceSelector vkb_physical_device_selector{vkb_instance};
    vkb_physical_device_selector.set_surface(anvilSurface);
    vkb_physical_device_selector.set_minimum_version(1, 3);
    vkb_physical_device_selector.add_required_extension_features(features13);
    vkb_physical_device_selector.add_required_extension_features(features11);
    vkb::Result<vkb::PhysicalDevice> vkb_physical_device_result = vkb_physical_device_selector.select();

    if (!vkb_physical_device_result)
    {
        std::ostringstream error_stream;
        error_stream << "Failed to select a suitable GPU physical device:\n"
                    << "  Primary error: " << vkb_physical_device_result.error().message() << "\n";

        const auto& failure_reasons = vkb_physical_device_result.detailed_failure_reasons();
        if (!failure_reasons.empty())
        {
            error_stream << "  Detailed failure reasons:\n";
            for (const auto& reason : failure_reasons)
            {
                error_stream << "    - " << reason << "\n";
            }
        }
        throw std::runtime_error(error_stream.str());
    }

    const vkb::PhysicalDevice& vkb_physical_device = vkb_physical_device_result.value();
    anvilPhysicalDevice = vkb_physical_device.physical_device;

    // --------------------------------
    // Build Logical Device
    vkb::DeviceBuilder vkb_device_builder{vkb_physical_device};
    vkb::Result<vkb::Device> vkb_device_result = vkb_device_builder.build();

    if (!vkb_device_result)
    {
        std::ostringstream error_stream;
        error_stream << "Failed to build logical Vulkan device:\n"
                    << "  Primary error: " << vkb_device_result.error().message();
        throw std::runtime_error(error_stream.str());
    }

    const vkb::Device& vkb_device = vkb_device_result.value();
    anvilDevice = vkb_device.device;

    // --------------------------------
    // Load Device functions into Volk
    volkLoadDevice(anvilDevice);

    // --------------------------------
    // Get Queues
    anvilGraphicsQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    anvilGraphicsQueueIndex = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    // --------------------------------
    // Initialise Vulkan Memory Allocator
    VmaVulkanFunctions vma_vulkan_functions = {};
    vma_vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vma_vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocator_create_info = {};
    allocator_create_info.physicalDevice = anvilPhysicalDevice;
    allocator_create_info.device = anvilDevice;
    allocator_create_info.instance = anvilInstance;
    allocator_create_info.pVulkanFunctions = &vma_vulkan_functions;
    allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_3;

    const VkResult vma_result = vmaCreateAllocator(&allocator_create_info, &anvilAllocator);
    if (vma_result != VK_SUCCESS)
    {
        throw std::runtime_error(std::string("Failed to create Vulkan Memory Allocator. VkResult: ") + AnvilResult::ToString(vma_result));
    }

    // --------------------------------
    // Initialize pool and fence for ImmediateSubmit logic
    VkCommandPoolCreateInfo upload_pool_info{};
    upload_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    upload_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    upload_pool_info.queueFamilyIndex = anvilGraphicsQueueIndex;

    const VkResult pool_result = vkCreateCommandPool(anvilDevice, &upload_pool_info, nullptr, &uploadCommandPool);
    if (pool_result != VK_SUCCESS)
    {
        throw std::runtime_error(std::string("Failed to create upload command pool. VkResult: ") + AnvilResult::ToString(pool_result));
    }

    VkFenceCreateInfo upload_fence_info{};
    upload_fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    const VkResult fence_result = vkCreateFence(anvilDevice, &upload_fence_info, nullptr, &uploadFence);
    if (fence_result != VK_SUCCESS)
    {
        throw std::runtime_error(std::string("Failed to create upload fence. VkResult: ") + AnvilResult::ToString(fence_result));
    }

    std::cout << "Finished initializing AnvilVulkanContext" << std::endl;
}

void AnvilVulkanContext::immediateSubmit(std::function<void(VkCommandBuffer inCmd)>&& callbackFunction) const
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
