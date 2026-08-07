// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_VULKANCONFIG_H
#define ANVIL_VK_VULKANCONFIG_H

/**
 * @file VulkanConfig.h
 * @brief Centralized configuration constants for Vulkan API versions and global settings.
 */

#include <cstdint>
#include <volk.h>

namespace AnvilVulkan
{
    // Target Vulkan API Version Components
    constexpr uint32_t API_VERSION_MAJOR = 1;
    constexpr uint32_t API_VERSION_MINOR = 3;
    constexpr uint32_t API_VERSION_PATCH = 0;

    constexpr uint32_t API_VERSION = VK_MAKE_API_VERSION(0, API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);
}

#endif //ANVIL_VK_VULKANCONFIG_H
