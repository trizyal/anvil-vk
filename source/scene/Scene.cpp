// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "Scene.h"

#include "VulkanContext.h"

Scene::~Scene()
{
    sceneUBO.destroyBuffer();
}

void Scene::createScene(VulkanContext& inContext)
{
    pContext = &inContext;

    // Create the UBO
    sceneUBO.createBuffer(
        *pContext,
        &data,
        sizeof(GlobalSceneData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    );

    isDirty = false;
}

void Scene::setGPUSceneData(const GlobalSceneData& inData)
{
    data = inData;
    isDirty = true;
}

void Scene::updateGPUBuffer()
{
    if (!isDirty || sceneUBO.buffer == VK_NULL_HANDLE)
    {
        return;
    }

    // Use VMA mapped memory to instantly update the lighting values on the GPU
    if (sceneUBO.allocation != VK_NULL_HANDLE)
    {
        vmaCopyMemoryToAllocation(pContext->allocator, &data, sceneUBO.allocation, 0, sizeof(GlobalSceneData));
        isDirty = false;
    }
}
