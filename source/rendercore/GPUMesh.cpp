// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "GPUMesh.h"

#include "ModelLoader.h"

GPUMesh::GPUMesh(GPUMesh&& other) noexcept
{
    *this = std::move(other);
}

GPUMesh& GPUMesh::operator=(GPUMesh&& other) noexcept
{
    if (this != &other)
    {
        vertexBuffer = std::move(other.vertexBuffer);
        indexBuffer = std::move(other.indexBuffer);

        indexCount = other.indexCount;

        ptrAContext = other.ptrAContext;

        // other.vertexBuffer.destroyBuffer();
        // other.indexBuffer.destroyBuffer();
        other.indexCount = 0;
        other.ptrAContext = nullptr;
    }
    return *this;
}

void GPUMesh::createGPUMesh(const VulkanContext& inContext, const CPUMesh_Single& inMesh)
{
    this->ptrAContext = &inContext;

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

void GPUMesh::createGPUMesh(const VulkanContext& inContext, const CPUMeshPrimitive& inMeshPrimitive)
{
    this->ptrAContext = &inContext;

    indexCount = static_cast<uint32_t>(inMeshPrimitive.indices.size());

    vertexBuffer.createBuffer(
        inContext.anvilAllocator,
        inContext.anvilDevice,
        inMeshPrimitive.vertices.data(),
        inMeshPrimitive.vertices.size() * sizeof(MeshVertex),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    );

    indexBuffer.createBuffer(
        inContext.anvilAllocator,
        inContext.anvilDevice,
        inMeshPrimitive.indices.data(),
        inMeshPrimitive.indices.size() * sizeof(uint32_t),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    );
}

void GPUMesh::destroyGPUMesh()
{
    vertexBuffer.destroyBuffer();
    indexBuffer.destroyBuffer();
}

VkVertexInputBindingDescription GPUMesh::getBindingDescription()
{
    VkVertexInputBindingDescription binding_description{};
    binding_description.binding = 0;
    binding_description.stride = sizeof(MeshVertex);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return binding_description;
}

std::array<VkVertexInputAttributeDescription, 3> GPUMesh::getAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};

    // 0: Position
    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(MeshVertex, position);

    // 1: Color
    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(MeshVertex, normal);

    // 2: UV
    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[2].offset = offsetof(MeshVertex, uv);

    return attribute_descriptions;
}
