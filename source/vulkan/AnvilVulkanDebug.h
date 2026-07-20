// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_VULKANDEBUG_H
#define ANVIL_VK_VULKANDEBUG_H

#include <string>
#include <source_location>

#include <volk.h>

namespace AnvilDebug
{
    // Vulkan validation callback
    VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    // Vulkan object naming
    void SetObjectName(
        VkDevice inDevice,
        uint64_t inObjectHandle,
        VkObjectType inObjectType,
        const char* inDebugName);

    // Smart context analyzer
    void SetAutoName(
        VkDevice inDevice,
        uint64_t inObjectHandle,
        VkObjectType inObjectType,
        const char* inName = nullptr,
        std::source_location location = std::source_location::current());

    // Type deducer to avoid casting Vulkan handles manually
    template <typename T>
    inline void SetAutoName(
        VkDevice inDevice,
        T inObjectHandle,
        VkObjectType inObjectType,
        const char* inName = nullptr,
        std::source_location location = std::source_location::current())
    {
        // Double cast prevents warnings across different OS handle architectures
        SetAutoName(inDevice, (uint64_t)(size_t)inObjectHandle, inObjectType, inName, location);
    }

    const char* ObjectTypeToString(VkObjectType inObjectType);
}

#endif //ANVIL_VK_VULKANDEBUG_H
