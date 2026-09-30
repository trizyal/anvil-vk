// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#define VOLK_IMPLEMENTATION
#include <volk.h>

#include "Ensure.h"

#ifndef NDEBUG
    // Fills new allocations with a bit pattern to catch uninitialized memory reads
    #define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1

    // Adds a 16-byte margin around allocations to detect buffer overruns/underruns
    #define VMA_DEBUG_MARGIN 16
    #define VMA_DEBUG_DETECT_CORRUPTION 1

    #if UNIMPLEMENTED
    #include <cstdio>

    // Routes VMA's internal leak tracking and warnings to standard error
    #define VMA_DEBUG_LOG(format, ...) do { \
        fprintf(stderr, "[VMA] " format "\n", __VA_ARGS__); \
        } while(false)
    #endif // UNIMPLEMENTED

    // Pipe VMA corruption checks to your engine's crash handler
    #define VMA_ASSERT(expr) FATAL((expr), "VMA Internal Assertion Failed: " #expr)
#endif // NDEBUG

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "VulkanContext.h"

#include <sstream>

#include <VkBootstrap.h>

#include "DebugNames.h"
#include "VulkanResult.h"
#include "Window.h"
#include "VulkanConfig.h"
#include "Logger.h"
#include "Trace.h"

void VulkanContext::initializeVulkanContext(Window& inWindow)
{
    SCOPE_CPU;
    LOG_TRACE("Initializing VulkanContext");

    pWindow = &inWindow;

    // --------------------------------
    // Initialise Volk
    LOG_TRACE("Initializing volk.");
    CHECK(volkInitialize());

    // --------------------------------
    // Create Instance
    vkb::InstanceBuilder vkb_instance_builder;
    vkb_instance_builder.set_app_name(inWindow.getWindowTitle().c_str());
    vkb_instance_builder.require_api_version(AnvilVulkan::API_VERSION);

#ifndef NDEBUG
    LOG_DEBUG("Requesting Validation Layers and Debug Callback.");

    vkb_instance_builder.request_validation_layers(true);
    vkb_instance_builder.set_debug_callback(VulkanDebug::DebugCallback);

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

#if 0
    VkValidationFeatureEnableEXT enables[] = {VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT};
    vkb_instance_builder.add_validation_feature_enable(enables[0]);
#endif

    // TODO: implement something like vkEnumerateInstanceExtensionProperties and then get available extensions.
    // vkb_instance_builder.enable_extension(VK_EXT_DEVICE_ADDRESS_BINDING_REPORT_EXTENSION_NAME);
#endif // NDEBUG

    vkb::Result<vkb::Instance> vkb_instance_result = vkb_instance_builder.build();
    VKB_CHECK(vkb_instance_result);

    LOG_TRACE("Creating vulkan instance.");
    const vkb::Instance vkb_instance = vkb_instance_result.value();
    instance = vkb_instance.instance;
    debugMessenger = vkb_instance.debug_messenger;

    // --------------------------------
    // Load Instance functions into Volk
    volkLoadInstance(instance);

    // --------------------------------
    // Create Surface
    surface = inWindow.createSurface(instance);

    // --------------------------------
    // Select Physical Device
    LOG_TRACE("Selecting vulkan physical device.");
    VkPhysicalDeviceFeatures base_features{};
    base_features.samplerAnisotropy = VK_TRUE;
    base_features.fillModeNonSolid = VK_TRUE; // Enable wireframe support

    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;

    // Fix for VUID-VkShaderModuleCreateInfo-pCode-08740
    VkPhysicalDeviceVulkan11Features features11{};
    features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    features11.shaderDrawParameters = VK_TRUE;

    vkb::PhysicalDeviceSelector vkb_physical_device_selector{vkb_instance};
    vkb_physical_device_selector.set_surface(surface);
    vkb_physical_device_selector.set_minimum_version(AnvilVulkan::API_VERSION_MAJOR, AnvilVulkan::API_VERSION_MINOR);
    vkb_physical_device_selector.set_required_features(base_features);
    vkb_physical_device_selector.add_required_extension_features(features13);
    vkb_physical_device_selector.add_required_extension_features(features11);
    vkb::Result<vkb::PhysicalDevice> vkb_physical_device_result = vkb_physical_device_selector.select();

    if (!vkb_physical_device_result)
    {
        // Cannot use VKB_CHECK as we do a detailed failure reason log here
        std::ostringstream error_stream;
        error_stream << "Failed to select a suitable GPU physical device:" << std::endl;
        error_stream << "   Primary error: " << vkb_physical_device_result.error().message() << std::endl;

        const auto& failure_reasons = vkb_physical_device_result.detailed_failure_reasons();
        if (!failure_reasons.empty())
        {
            error_stream << "  Detailed failure reasons:" << std::endl;
            for (const auto& reason : failure_reasons)
            {
                error_stream << "    - " << reason << std::endl;
            }
        }

        LOG_FATAL("{}", error_stream.str());
        FATAL(false, "Failed to select physical device.");
    }

    const vkb::PhysicalDevice& vkb_physical_device = vkb_physical_device_result.value();
    physicalDevice = vkb_physical_device.physical_device;
    physicalDeviceProperties = vkb_physical_device.properties;

    // --------------------------------
    // Build Logical Device
    LOG_TRACE("Creating vulkan logical device.");
    vkb::DeviceBuilder vkb_device_builder{vkb_physical_device};
    vkb::Result<vkb::Device> vkb_device_result = vkb_device_builder.build();
    VKB_CHECK(vkb_device_result);

    const vkb::Device& vkb_device = vkb_device_result.value();
    device = vkb_device.device;

    // --------------------------------
    // Load Device functions into Volk
    volkLoadDevice(device);

    // --------------------------------
    // Get Queues
    graphicsQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    graphicsQueueIndex = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    // --------------------------------
    // Initialise Vulkan Memory Allocator
    LOG_TRACE("Creating vulkan memory allocator.");
    VmaVulkanFunctions vma_vulkan_functions = {};
    vma_vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vma_vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocator_create_info = {};
    allocator_create_info.physicalDevice = physicalDevice;
    allocator_create_info.device = device;
    allocator_create_info.instance = instance;
    allocator_create_info.pVulkanFunctions = &vma_vulkan_functions;
    allocator_create_info.vulkanApiVersion = AnvilVulkan::API_VERSION;
    CHECK(vmaCreateAllocator(&allocator_create_info, &allocator));

    // --------------------------------
    // Initialize pool and fence for ImmediateSubmit logic
    LOG_TRACE("Creating vulkan command pool for ImmediateSubmit.");
    VkCommandPoolCreateInfo upload_pool_info{};
    upload_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    upload_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    upload_pool_info.queueFamilyIndex = graphicsQueueIndex;
    CHECK(vkCreateCommandPool(device, &upload_pool_info, nullptr, &uploadCommandPool));

    LOG_TRACE("Creating vulkan fence for ImmediateSubmit.");
    VkFenceCreateInfo upload_fence_info{};
    upload_fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    CHECK(vkCreateFence(device, &upload_fence_info, nullptr, &uploadFence));

    LOG_TRACE("Finished Initializing VulkanContext");
}

void VulkanContext::immediateSubmit(std::function<void(VkCommandBuffer inCmd)>&& callbackFunction) const
{
    SCOPE_CPU;
    
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = uploadCommandPool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &alloc_info, &cmd);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &begin_info);
    callbackFunction(cmd);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    vkQueueSubmit(graphicsQueue, 1, &submit_info, uploadFence);
    vkWaitForFences(device, 1, &uploadFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &uploadFence);

    vkFreeCommandBuffers(device, uploadCommandPool, 1, &cmd);
}

VulkanContext::~VulkanContext()
{
    vkDestroyFence(device, uploadFence, nullptr);
    vkDestroyCommandPool(device, uploadCommandPool, nullptr);
    vmaDestroyAllocator(allocator);

    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
#ifndef NDEBUG
    vkb::destroy_debug_utils_messenger(instance, debugMessenger);
#endif
    vkDestroyInstance(instance, nullptr);
}
