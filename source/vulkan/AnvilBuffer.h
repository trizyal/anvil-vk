// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_BUFFER_H
#define ANVIL_VK_BUFFER_H

#include <volk.h>
#include <vk_mem_alloc.h>

class AnvilBuffer
{
public:
    AnvilBuffer() = default;
    ~AnvilBuffer() = default;

    // Disallow copying to prevent double destruction/creation
    AnvilBuffer(const AnvilBuffer&) = delete;
    AnvilBuffer& operator=(const AnvilBuffer&) = delete;

    // Allow moving
    AnvilBuffer(AnvilBuffer&& other) noexcept;
    AnvilBuffer& operator=(AnvilBuffer&& other) noexcept;

// private:
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

public:
    void createAndUpload(VmaAllocator inAllocator, const void* inData, VkDeviceSize size, VkBufferUsageFlags usage);
    void destroy(VmaAllocator inAllocator);
    const VkBuffer* get() const { return &buffer; }
};

#endif //ANVIL_VK_BUFFER_H
