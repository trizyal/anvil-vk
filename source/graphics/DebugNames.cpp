// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "DebugNames.h"

#include <iostream>
#include <unordered_map>
#include <mutex>
#include <filesystem>
#include <sstream>
#include <cstring>

namespace VulkanDebug
{
    VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* /*pUserData*/)
    {
        // TODO: Message severity should be toggleable.
        if (messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            return VK_FALSE;
        }

        // TODO: Add handling of Message types.

        static std::unordered_map<int32_t, int> error_counts;
        static std::mutex error_mutex;
        const int MAX_PRINTS = 3; // Change this to view an error a few times before silencing.

        std::lock_guard<std::mutex> lock(error_mutex);
        int& count = error_counts[pCallbackData->messageIdNumber];

        if (count < MAX_PRINTS) {
            std::cerr << "[Vulkan Validation] " << pCallbackData->pMessageIdName << "\n";

            // Extract and print all debug names
            if (pCallbackData->objectCount > 0)
            {
                std::cerr << "Involved Objects:\n";
                for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
                {
                    const auto& obj = pCallbackData->pObjects[i];
                    std::cerr << "  - [" << i << "] "
                              << "Name: " << (obj.pObjectName ? obj.pObjectName : "<Unnamed>")
                              << " | Handle: 0x" << std::hex << obj.objectHandle << std::dec << "\n";
                }
            }

            std::cerr << "Message:\n" << pCallbackData->pMessage << "\n" << std::endl;
        }
        else if (count == MAX_PRINTS) {
            std::cerr << "[Vulkan Validation] (Suppressing further prints of: "
                      << pCallbackData->pMessageIdName << ")\n" << std::endl;
        }

        count++;
        return VK_FALSE;
    }

    void SetObjectName(VkDevice inDevice, uint64_t inObjectHandle, VkObjectType inObjectType, const char* inDebugName)
    {
        if (!inDevice || !inObjectHandle || !vkSetDebugUtilsObjectNameEXT)
        {
            return;
        }

        VkDebugUtilsObjectNameInfoEXT debug_name_info{};
        debug_name_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        debug_name_info.objectType = inObjectType;
        debug_name_info.objectHandle = inObjectHandle;
        debug_name_info.pObjectName = inDebugName;

        vkSetDebugUtilsObjectNameEXT(inDevice, &debug_name_info);
    }

    void SetAutoName(VkDevice inDevice, uint64_t inObjectHandle, VkObjectType inObjectType,
        const char* inName, std::source_location location)
    {
        std::string final_name;

        // Extract file name
        std::string short_file_name = std::filesystem::path(location.file_name()).filename().string();

        if (inName && std::strlen(inName) > 0)
        {
            std::stringstream ss;
            ss << inName << " [" << short_file_name << ":" << location.line() << "]";
            final_name = ss.str();
        }
        else
        {
            std::stringstream ss;
            ss << "AutoName_" << ObjectTypeToString(inObjectType) << " [" << short_file_name << ":" << location.line() << "]";
            final_name = ss.str();
        }

        SetObjectName(inDevice, inObjectHandle, inObjectType, final_name.c_str());
    }

    const char* ObjectTypeToString(VkObjectType inObjectType)
    {
        switch (inObjectType)
        {
        case VK_OBJECT_TYPE_BUFFER: return "Buffer";
        case VK_OBJECT_TYPE_COMMAND_BUFFER: return "CommandBuffer";
        case VK_OBJECT_TYPE_COMMAND_POOL: return "CommandPool";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL: return "DescriptorPool";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET: return "DescriptorSet";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT: return "DescriptorSetLayout";
        case VK_OBJECT_TYPE_DEVICE: return "Device";
        case VK_OBJECT_TYPE_FENCE: return "Fence";
        case VK_OBJECT_TYPE_FRAMEBUFFER: return "Framebuffer";
        case VK_OBJECT_TYPE_IMAGE: return "Image";
        case VK_OBJECT_TYPE_IMAGE_VIEW: return "ImageView";
        case VK_OBJECT_TYPE_INSTANCE: return "Instance";
        case VK_OBJECT_TYPE_PHYSICAL_DEVICE: return "PhysicalDevice";
        case VK_OBJECT_TYPE_PIPELINE: return "Pipeline";
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT: return "PipelineLayout";
        case VK_OBJECT_TYPE_RENDER_PASS: return "RenderPass";
        case VK_OBJECT_TYPE_SAMPLER: return "Sampler";
        case VK_OBJECT_TYPE_SEMAPHORE: return "Semaphore";
        case VK_OBJECT_TYPE_SHADER_MODULE: return "ShaderModule";
        case VK_OBJECT_TYPE_SURFACE_KHR: return "Surface";
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR: return "Swapchain";
        default: return "Unknown";
        }
    }

}
