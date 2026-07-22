// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_VULKANDEBUG_H
#define ANVIL_VK_VULKANDEBUG_H

#include <string>
#include <source_location>

#include <volk.h>

// Automatically enable in Debug builds, disable in Release builds
#ifndef NDEBUG
#define ANVIL_DEBUG 1
#else
#define ANVIL_DEBUG 0
#endif

// The comma-injection trick: strips arguments entirely if disabled
#if ANVIL_DEBUG
#define ANVIL_DEBUG_ARG(x), x
#else
#define ANVIL_DEBUG_ARG(x)
#endif

#define ANVIL_DEBUG_DECL() \
    ANVIL_DEBUG_ARG(const char* aDebugName = nullptr) \
    ANVIL_DEBUG_ARG(std::source_location const aDbgSrcLoc = std::source_location::current())

#define ANVIL_DEBUG_DEFN \
    ANVIL_DEBUG_ARG(const char* aDebugName) \
    ANVIL_DEBUG_ARG(std::source_location const aDbgSrcLoc)

#if ANVIL_DEBUG
#   define ANVIL_DEBUG_NAME(dev, handle, type) \
    AnvilDebug::SetAutoName(dev, (uint64_t)handle, type, aDebugName, aDbgSrcLoc)
#else
#   define ANVIL_DEBUG_NAME(dev, hendle, type) do {} while (0)
#endif

// Optional: For passing the variable name automatically (e.g., ANVIL_NAME_OF(vertexBuffer))
#define ANVIL_NAME_OF(var) #var

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
