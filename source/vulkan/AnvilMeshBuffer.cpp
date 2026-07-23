// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilMeshBuffer.h"

#include "AnvilMeshLoader.h"
#include "AnvilVulkanContext.h"

void AnvilMeshBuffer::createAnvilMeshBuffer(const AnvilVulkanContext& inContext, const AnvilMesh& inMesh)
{
    indexCount = static_cast<uint32_t>(inMesh.indices.size());

    // TODO: Need to get debug name somehow
    vertexBuffer.createBuffer(
        inContext.anvilAllocator,
        inContext.anvilDevice,
        inMesh.vertices.data(),
        inMesh.vertices.size() * sizeof(MeshVertex),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    );

    indexBuffer.createBuffer(
        inContext.anvilAllocator,
        inContext.anvilDevice,
        inMesh.indices.data(),
        inMesh.indices.size() * sizeof(uint32_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    );
}

void AnvilMeshBuffer::destroyAnvilMeshBuffer(const AnvilVulkanContext& inContext)
{
    vertexBuffer.destroyBuffer(inContext.anvilAllocator);
    indexBuffer.destroyBuffer(inContext.anvilAllocator);
}

VkVertexInputBindingDescription AnvilMeshBuffer::getBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(MeshVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> AnvilMeshBuffer::getAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(MeshVertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(MeshVertex, color);

    return attributeDescriptions;
}
