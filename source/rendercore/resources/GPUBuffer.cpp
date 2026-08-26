// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "GPUBuffer.h"

#include <stdexcept>
#include <cstring>
#include <utility>

#include "DebugNames.h"
#include "VulkanContext.h"

GPUBuffer::GPUBuffer(GPUBuffer&& other) noexcept
{
    *this = std::move(other);
}

GPUBuffer& GPUBuffer::operator=(GPUBuffer&& other) noexcept
{
    if (this != &other)
    {
        destroyBuffer();

        buffer = other.buffer;
        allocation = other.allocation;
        allocator = other.allocator;

        other.buffer = VK_NULL_HANDLE;
        other.allocation = VK_NULL_HANDLE;
        other.allocator = nullptr;   // Optional, but good practice
    }
    return *this;
}

void GPUBuffer::createBuffer(const VulkanContext& inContext, const void* inData, VkDeviceSize size, VkBufferUsageFlags usage
    D_DEFN)
{
    // Clean up if this object wrapper is being reused
    if (buffer != VK_NULL_HANDLE)
    {
        destroyBuffer();
    }

    allocator = inContext.allocator;

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; // CPU_TO_GPU for dynamic/staging data as it ensures host-visibility

    if (vmaCreateBuffer(allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Anvil Engine: Failed to allocate VMA buffer.");
    }

    SET_DNAME(inContext.device, buffer, VK_OBJECT_TYPE_BUFFER);

    // Direct memory mapping and immediate transfer
    void* mapped_memory = nullptr;
    vmaMapMemory(allocator, allocation, &mapped_memory);
    std::memcpy(mapped_memory, inData, size);
    vmaUnmapMemory(allocator, allocation);
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
