// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_BUFFER_H
#define ANVIL_VK_BUFFER_H

/**
 * @file AnvilBuffer.h
 * @brief Move-only wrapper around Vulkan buffers and Vulkan Memory Allocator (VMA) allocations.
 */

#include <source_location>

#include <volk.h>
#include <vk_mem_alloc.h>

#include "../context/AnvilVulkanDebug.h"

/**
 * @brief Manages the lifecycle of a GPU Vulkan buffer and its backing VMA memory allocation.
 *
 * Implements move-only semantics to safely transfer buffer ownership across scopes
 * and STL containers without risking accidental double-free memory corruption.
 *
 * @note Because GPU memory destruction often requires synchronization with the render loop,
 * the destructor does not automatically free GPU memory. You must call destroyBuffer() explicitly.
 *
 * @note Copying this class is disallowed. Moving is allowed.
 */
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

    /** Underlying Vulkan buffer handle. */
    VkBuffer buffer = VK_NULL_HANDLE;

    /** Associated VMA memory allocation handle. */
    VmaAllocation allocation = VK_NULL_HANDLE;

private:
    /** Cached VMA allocator used for self-destruction. */
    VmaAllocator allocator = VK_NULL_HANDLE;

public:
    /**
     * @brief Allocates GPU memory and creates a Vulkan buffer.
     *
     * @param inAllocator VMA allocator instance used to allocate GPU memory and cached for cleanup.
     * @param inDevice Logical Vulkan device handle.
     * @param inData Pointer to CPU source data to copy into the buffer (may be nullptr).
     * @param size Size of the buffer in bytes.
     * @param usage Bitmask of VkBufferUsageFlags specifying intended buffer operations.
     *
     * @throws std::runtime_error If buffer allocation or GPU transfer commands fail.
     */
    void createBuffer(VmaAllocator inAllocator, VkDevice inDevice, const void* inData, VkDeviceSize size, VkBufferUsageFlags usage
                      ANVIL_DEBUG_DECL());

    /**
     * @brief Releases the underlying Vulkan buffer and frees the associated VMA allocation.
     *
     * @note Uses the internally cached VMA allocator. Safe to call multiple times or on zeroed/null handles.
     */
    void destroyBuffer();
};

#endif //ANVIL_VK_BUFFER_H
