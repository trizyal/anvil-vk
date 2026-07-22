// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilMeshBuffer.h"

#include "AnvilMeshLoader.h"
#include "AnvilVulkanContext.h"

void AnvilMeshBuffer::createAnvilMeshBuffer(AnvilVulkanContext& inContext, const AnvilMesh& inMesh)
{
    indexCount = static_cast<uint32_t>(inMesh.indices.size());

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
