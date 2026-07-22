// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_MESH_H
#define ANVIL_VK_MESH_H

#include <array>
#include <glm/glm.hpp>
#include <volk.h>

#include "AnvilBuffer.h"

struct MeshVertex
{
    glm::vec3 position;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(MeshVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
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
};

struct AnvilMesh
{
    AnvilBuffer vertexBuffer;
    AnvilBuffer indexBuffer;
    uint32_t indexCount;

    void destroyMesh(VmaAllocator inAllocator)
    {
        vertexBuffer.destroyBuffer(inAllocator);
        indexBuffer.destroyBuffer(inAllocator);
    }
};

#endif //ANVIL_VK_MESH_H
