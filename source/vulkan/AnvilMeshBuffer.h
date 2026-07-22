// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_MESHBUFFER_H
#define ANVIL_VK_MESHBUFFER_H

#include <array>
#include <volk.h>
#include <glm/glm.hpp>

#include "AnvilBuffer.h"

struct AnvilMesh;
class AnvilVulkanContext;

struct AnvilMeshBuffer
{
    AnvilBuffer vertexBuffer;
    AnvilBuffer indexBuffer;
    uint32_t indexCount = 0;

    // Upload AnvilMesh to Buffer
    void createAnvilMeshBuffer(AnvilVulkanContext& inContext, const AnvilMesh& inMesh);
    void destroyAnvilMeshBuffer(AnvilVulkanContext& inContext);

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};

#endif //ANVIL_VK_MESHBUFFER_H
