// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilBuffer.h"

#include <stdexcept>
#include <cstring>
#include <utility>

#include "AnvilVulkanDebug.h"

AnvilBuffer::AnvilBuffer(AnvilBuffer&& other) noexcept
{
    *this = std::move(other);
}

AnvilBuffer& AnvilBuffer::operator=(AnvilBuffer&& other) noexcept
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

void AnvilBuffer::createAndUpload(VmaAllocator inAllocator, VkDevice inDevice, const void* inData, VkDeviceSize size, VkBufferUsageFlags usage,
    const char* debugName, std::source_location loc)
{
    // Clean up if this object wrapper is being reused
    if (buffer != VK_NULL_HANDLE)
    {
        destroy(inAllocator);
    }

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; // CPU_TO_GPU for dynamic/staging data as it ensures host-visibility

    if (vmaCreateBuffer(inAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Anvil Engine: Failed to allocate VMA buffer.");
    }

    AnvilDebug::SetAutoName(inDevice, buffer, VK_OBJECT_TYPE_BUFFER, debugName, loc);

    // Direct memory mapping and immediate transfer
    void* mappedMemory = nullptr;
    vmaMapMemory(inAllocator, allocation, &mappedMemory);
    std::memcpy(mappedMemory, inData, size);
    vmaUnmapMemory(inAllocator, allocation);
}

void AnvilBuffer::destroy(VmaAllocator inAllocator)
{
    if (buffer != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(inAllocator, buffer, allocation);
        buffer = VK_NULL_HANDLE;
        allocation = VK_NULL_HANDLE;
    }
}
