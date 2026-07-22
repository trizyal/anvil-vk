// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_BUFFER_H
#define ANVIL_VK_BUFFER_H

#include <source_location>

#include <volk.h>
#include <vk_mem_alloc.h>

#include "AnvilVulkanDebug.h"

class AnvilBuffer
{
public:
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    AnvilBuffer() = default;
    ~AnvilBuffer() = default;

    // Disallow copying to prevent double destruction/creation
    AnvilBuffer(const AnvilBuffer&) = delete;
    AnvilBuffer& operator=(const AnvilBuffer&) = delete;

    // Allow moving
    AnvilBuffer(AnvilBuffer&& other) noexcept;
    AnvilBuffer& operator=(AnvilBuffer&& other) noexcept;

    void createBuffer(VmaAllocator inAllocator, VkDevice inDevice, const void* inData, VkDeviceSize size, VkBufferUsageFlags usage
        ANVIL_DEBUG_DECL());
    void destroyBuffer(VmaAllocator inAllocator);
};

#endif //ANVIL_VK_BUFFER_H
