// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "GPUModel.h"

GPUModel::GPUModel(GPUModel&& other) noexcept
{
    *this = std::move(other);
}

GPUModel& GPUModel::operator=(GPUModel&& other) noexcept
{
    if (this != &other)
    {
        destroyGPUModel();

        pContext = other.pContext;
        textures = std::move(other.textures);
        gpuMeshes = std::move(other.gpuMeshes);
        gpuMaterials = std::move(other.gpuMaterials);
        drawItems = std::move(other.drawItems);

        other.pContext = nullptr;
    }
    return *this;
}
