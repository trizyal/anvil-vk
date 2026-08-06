// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "GPUBuffer.h"

#include <stdexcept>
#include <cstring>
#include <utility>

#include "AnvilVulkanDebug.h"

GPUBuffer::GPUBuffer(GPUBuffer&& other) noexcept
{
    *this = std::move(other);
}

GPUBuffer& GPUBuffer::operator=(GPUBuffer&& other) noexcept
{
    if (this != &other)
    {
        buffer = other.buffer;
        allocation = other.allocation;

        other.buffer = VK_NULL_HANDLE;
        other.allocation = VK_NULL_HANDLE;
    }
    return *this;
}

void GPUBuffer::createBuffer(VmaAllocator inAllocator, VkDevice inDevice, const void* inData, VkDeviceSize size, VkBufferUsageFlags usage
    ANVIL_DEBUG_DEFN)
{
    // Clean up if this object wrapper is being reused
    if (buffer != VK_NULL_HANDLE)
    {
        destroyBuffer();
    }

    allocator = inAllocator;

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; // CPU_TO_GPU for dynamic/staging data as it ensures host-visibility

    if (vmaCreateBuffer(inAllocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Anvil Engine: Failed to allocate VMA buffer.");
    }

    ANVIL_DEBUG_NAME(inDevice, buffer, VK_OBJECT_TYPE_BUFFER);

    // Direct memory mapping and immediate transfer
    void* mapped_memory = nullptr;
    vmaMapMemory(inAllocator, allocation, &mapped_memory);
    std::memcpy(mapped_memory, inData, size);
    vmaUnmapMemory(inAllocator, allocation);
}

void GPUBuffer::destroyBuffer()
{
    if (buffer != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(allocator, buffer, allocation);
        buffer = VK_NULL_HANDLE;
        allocation = VK_NULL_HANDLE;
    }
}
