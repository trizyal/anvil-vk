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
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
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
            std::cerr << "[Vulkan Validation] " << pCallbackData->pMessageIdName << ":\n"
                      << pCallbackData->pMessage << "\n" << std::endl;
        } else if (count == MAX_PRINTS) {
            std::cerr << "[Vulkan Validation] (Suppressing further prints of: "
                      << pCallbackData->pMessageIdName << ")\n" << std::endl;
        }

        count++;
        return VK_FALSE;
    }

    void SetObjectName(VkDevice inDevice, uint64_t inObjectHandle, VkObjectType inObjectType, const char* inDebugName)
    {

    }

    void SetAutoName(VkDevice inDevice, uint64_t inObjectHandle, VkObjectType inObjectType,
        const char* inName, std::source_location location)
    {

    }

}
