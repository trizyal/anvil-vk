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
    ptrContext = &inContext;

    // Create the UBO
    sceneUBO.createBuffer(
        ptrContext->anvilAllocator,
        ptrContext->anvilDevice,
        &data,
        sizeof(GPUSceneData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    );

    isDirty = false;
}

void Scene::setGPUSceneData(const GPUSceneData& inData)
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
        vmaCopyMemoryToAllocation(ptrContext->anvilAllocator, &data, sceneUBO.allocation, 0, sizeof(GPUSceneData));
        isDirty = false;
    }
}
