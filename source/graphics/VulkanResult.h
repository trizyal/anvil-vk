// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_VULKANRESULT_H
#define ANVIL_VK_VULKANRESULT_H

/**
 * @file VulkanResult.h
 * @brief Error handling and diagnostic utilities for checking Vulkan API return codes.
 */

#include <string>

#include <volk.h>

/**
 * @brief Utilities for translating and verifying Vulkan API return codes.
 */
namespace VulkanResult
{
    /**
     * @brief Converts a Vulkan VkResult error code into a human-readable string literal.
     *
     * @see https://docs.vulkan.org/refpages/latest/refpages/source/VkResult.html
     *
     * @param aResult The Vulkan return code to decode.
     * @return A string matching the Vulkan enum name (e.g., "VK_ERROR_OUT_OF_DEVICE_MEMORY").
     */
    std::string ToString(VkResult aResult);

    /**
     * @brief Verifies that a Vulkan API operation succeeded.
     *
     * If `aResult` evaluates to an error code, this function formats a descriptive diagnostic
     * message containing the Vulkan error string, the literal function call expression, and the
     * originating file and line number before aborting.
     *
     * @param aResult      The Vulkan return code evaluated from an API call.
     * @param functionName Stringified representation of the evaluated expression (e.g., "vkCreateDevice(...)").
     * @param file         Source filename where the check was executed (__FILE__).
     * @param line         Source line number where the check was executed (__LINE__).
     *
     * @note Aborts engine if VKResult is not VK_SUCCESS.
     */
    void CheckVulkanResult(VkResult aResult, const char* functionName, const char* file, int line);

    /**
     * @brief Verifies that a vk-bootstrap operation succeeded, crashing and logging on failure.
     *
     * If `aResult` evaluates to an error code, this function formats a descriptive diagnostic
     * message containing the Vulkan error string, the literal function call expression, and the
     * originating file and line number before aborting.
     */
    void CheckVkBootstrapResult(const std::string& errorMessage, const char* functionName, const char* file, int line);
} //VulkanResult

/**
 * @brief Macro wrapper around Vulkan API calls that automatically checks for errors and aborts on failure.
 *
 * Captures the exact source code text of the expression (`#x`), the current file (`__FILE__`), and the
 * line number (`__LINE__`) to provide precise stack-trace-style context in exception messages.
 *
 * @note Example: `CHECK(vkCreateFence(device, &fenceInfo, nullptr, &fence));`
 */
#define CHECK(x) VulkanResult::CheckVulkanResult((x), #x, __FILE__, __LINE__)

#define VKB_CHECK(x) \
    do \
    { \
        if (!x) \
        { \
            VulkanResult::CheckVkBootstrapResult(x.error().message(), #x, __FILE__, __LINE__); \
        } \
    } \
    while (0)

#endif //ANVIL_VK_VULKANRESULT_H
