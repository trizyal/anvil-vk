// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_VULKANDEBUG_H
#define ANVIL_VK_VULKANDEBUG_H

/**
 * @file AnvilVulkanDebug.h
 * @brief Vulkan validation callbacks, GPU object naming utilities, and zero-cost debug instrumentation macros.
 */

#include <string>
#include <source_location>

#include <volk.h>

/**
 * @brief Master compilation toggle for Vulkan debugging features.
 *
 * Automatically enabled in Debug builds (when NDEBUG is undefined) and stripped in Release builds.
 */
#ifndef NDEBUG
#   define ANVIL_DEBUG 1
#else
#   define ANVIL_DEBUG 0
#endif

namespace AnvilDebug
{
    /**
     * @brief Vulkan validation layer callback intercepting SDK debug messages.
     *
     * Registered with the Vulkan instance during initialization. Receives validation errors,
     * performance warnings, and verbose debugging info from the active validation layers.
     *
     * @param messageSeverity Severity level of the incoming message (e.g., Error, Warning, Info).
     * @param messageType     Category of the message (General, Validation, or Performance).
     * @param pCallbackData   Payload containing the diagnostic message string and involved GPU objects.
     * @param pUserData       Optional user-defined pointer passed during messenger registration.
     * @return Always returns VK_FALSE to indicate that the Vulkan API call should not be aborted.
     */
    VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    /**
     * @brief Low-level wrapper around `vkSetDebugUtilsObjectNameEXT` to assign human-readable labels to GPU handles.
     *
     * Labeled objects appear by name in graphics debuggers (RenderDoc, Nsight Graphics) and in
     * Vulkan validation layer error messages.
     *
     * @param[in] inDevice       Logical Vulkan device that created the object.
     * @param[in] inObjectHandle Raw 64-bit integer representation of the Vulkan handle.
     * @param[in] inObjectType   Vulkan object type enum (e.g., VK_OBJECT_TYPE_BUFFER, VK_OBJECT_TYPE_IMAGE).
     * @param[in] inDebugName    Null-terminated string to assign as the object label.
     */
    void SetObjectName(VkDevice inDevice, uint64_t inObjectHandle, VkObjectType inObjectType, const char* inDebugName);

    /**
     * @brief Smart object namer that applies an explicit name or generates a contextual fallback from source location.
     *
     * If `inName` is null or empty, automatically generates a descriptive label formatted from
     * the filename, line number, and function name captured by `std::source_location`.
     *
     * @param[in] inDevice       Logical Vulkan device that created the object.
     * @param[in] inObjectHandle Raw 64-bit integer representation of the Vulkan handle.
     * @param[in] inObjectType   Vulkan object type enum.
     * @param[in] inName         Optional explicit name. If nullptr, source location metadata is used instead.
     * @param[in] location       Automatic.
     */
    void SetAutoName(VkDevice inDevice, uint64_t inObjectHandle, VkObjectType inObjectType,
            const char* inName = nullptr, std::source_location location = std::source_location::current());


    /**
     * @brief Template helper allowing arbitrary Vulkan handles to be named without manual integer casting.
     *
     * Safely converts 32-bit/64-bit dispatchable and non-dispatchable Vulkan handles into the 64-bit
     * integer format required by the Vulkan debug utils API across different operating systems.
     *
     * @tparam T Vulkan handle type (e.g., VkBuffer, VkImage, VkShaderModule).
     * @param[in] inDevice       Logical Vulkan device that created the object.
     * @param[in] inObjectHandle Vulkan API handle to label.
     * @param[in] inObjectType   Vulkan object type enum.
     * @param[in] inName         Optional explicit label.
     * @param[in] location       Automatic.
     */
    template <typename T>
    inline void SetAutoName(VkDevice inDevice, T inObjectHandle, VkObjectType inObjectType,
            const char* inName = nullptr, std::source_location location = std::source_location::current())
    {
        // Double cast prevents warnings across different OS handle architectures
        SetAutoName(inDevice, (uint64_t)(size_t)inObjectHandle, inObjectType, inName, location);
    }

    /**
     * @brief Converts a Vulkan object type enum into a human-readable string literal.
     *
     * @param[in] inObjectType Vulkan object type enum value.
     * @return Null-terminated string matching the Vulkan enum name (e.g., "VK_OBJECT_TYPE_BUFFER").
     */
    const char* ObjectTypeToString(VkObjectType inObjectType);
} //AnvilDebug

/**
 * @brief Comma-injection helper that appends a trailing parameter in Debug builds, or strips it in Release builds.
 */
#if ANVIL_DEBUG
#define ANVIL_DEBUG_ARG(x), x
#else
#define ANVIL_DEBUG_ARG(x)
#endif

/**
 * @brief Injects optional trailing debug name and source location parameters into function declarations.
 * @note Use this inside header declarations: `bool createBuffer(VkDevice dev ANVIL_DEBUG_DECL());`
 */
#define ANVIL_DEBUG_DECL() \
ANVIL_DEBUG_ARG(const char* aDebugName = nullptr) \
ANVIL_DEBUG_ARG(std::source_location const aDbgSrcLoc = std::source_location::current())

/**
 * @brief Injects trailing debug name and source location parameters into function definitions (without default values).
 * @note Use this inside .cpp implementations: `bool createBuffer(VkDevice dev ANVIL_DEBUG_DEFN) { ... }`
 */
#define ANVIL_DEBUG_DEFN \
ANVIL_DEBUG_ARG(const char* aDebugName) \
ANVIL_DEBUG_ARG(std::source_location const aDbgSrcLoc)

/**
 * @brief Conditionally applies a debug label to a Vulkan handle using the injected debug arguments.
 *
 * Expands to a `SetAutoName` call in Debug builds, or a no-op `do {} while (0)` in Release builds.
 * Must be used inside functions that include `ANVIL_DEBUG_DEFN` in their signature.
 *
 * @param dev    Logical Vulkan device handle.
 * @param handle Created Vulkan resource handle (e.g., VkBuffer, VkImage).
 * @param type   Vulkan object type enum (e.g., VK_OBJECT_TYPE_BUFFER).
 */
#if ANVIL_DEBUG
#   define ANVIL_DEBUG_NAME(dev, handle, type) \
    AnvilDebug::SetAutoName(dev, (uint64_t)handle, type, aDebugName, aDbgSrcLoc)
#else
#   define ANVIL_DEBUG_NAME(dev, handle, type) do {} while (0)
#endif

/**
 * @brief Convenience macro that converts a C++ variable identifier into a string literal for debug naming.
 *
 * @note Example: `ANVIL_NAME_OF(vertexBuffer)`.
 */
#define ANVIL_NAME_OF(var) #var

#endif //ANVIL_VK_VULKANDEBUG_H
