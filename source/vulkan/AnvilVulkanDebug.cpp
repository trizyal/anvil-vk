// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilVulkanDebug.h"

#include <iostream>
#include <unordered_map>
#include <mutex>
#include <filesystem>
#include <sstream>
#include <cstring>

namespace AnvilDebug
{
    VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* /*pUserData*/)
    {
        if (messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            return VK_FALSE;
        }

        static std::unordered_map<int32_t, int> errorCounts;
        static std::mutex errorMutex;
        const int MAX_PRINTS = 1; // Change this to 3 if you want to see an error a few times before silencing

        std::lock_guard<std::mutex> lock(errorMutex);
        int& count = errorCounts[pCallbackData->messageIdNumber];

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

        VkDebugUtilsObjectNameInfoEXT debugNameInfo{};
        debugNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        debugNameInfo.objectType = inObjectType;
        debugNameInfo.objectHandle = inObjectHandle;
        debugNameInfo.pObjectName = inDebugName;

        vkSetDebugUtilsObjectNameEXT(inDevice, &debugNameInfo);
    }

    void SetAutoName(VkDevice inDevice, uint64_t inObjectHandle, VkObjectType inObjectType,
        const char* inName, std::source_location location)
    {
        std::string finalName;

        // Extract file name
        std::string shortFileName = std::filesystem::path(location.file_name()).filename().string();

        if (inName && std::strlen(inName) > 0)
        {
            std::stringstream ss;
            ss << inName << " [" << shortFileName << ":" << location.line() << "]";
            finalName = ss.str();
        }
        else
        {
            std::stringstream ss;
            ss << "AutoName_" << ObjectTypeToString(inObjectType) << " [" << shortFileName << ":" << location.line() << "]";
            finalName = ss.str();
        }

        SetObjectName(inDevice, inObjectHandle, inObjectType, finalName.c_str());
    }

    const char* ObjectTypeToString(VkObjectType inObjectType)
    {
        switch (inObjectType)
        {
        case VK_OBJECT_TYPE_BUFFER: return "Buffer";
        case VK_OBJECT_TYPE_IMAGE: return "Image";
        case VK_OBJECT_TYPE_IMAGE_VIEW: return "ImageView";
        case VK_OBJECT_TYPE_PIPELINE: return "Pipeline";
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT: return "PipelineLayout";
        case VK_OBJECT_TYPE_SHADER_MODULE: return "ShaderModule";
        case VK_OBJECT_TYPE_COMMAND_BUFFER: return "CommandBuffer";
        case VK_OBJECT_TYPE_COMMAND_POOL: return "CommandPool";
        case VK_OBJECT_TYPE_RENDER_PASS: return "RenderPass";
        case VK_OBJECT_TYPE_FRAMEBUFFER: return "Framebuffer";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT: return "DescriptorSetLayout";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL: return "DescriptorPool";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET: return "DescriptorSet";
        case VK_OBJECT_TYPE_SEMAPHORE: return "Semaphore";
        case VK_OBJECT_TYPE_FENCE: return "Fence";
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR: return "Swapchain";
        default: return "Unknown";
        }
    }

}
