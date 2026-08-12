// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "MaterialInstance.h"

#include <iostream>
#include <stdexcept>
#include "AnvilMaterial.h"
#include "VulkanContext.h"

MaterialInstance::MaterialInstance(MaterialInstance&& other) noexcept
{
    *this = std::move(other);
}

MaterialInstance& MaterialInstance::operator=(MaterialInstance&& other) noexcept
{
    if (this != &other)
    {
        pContext = other.pContext;
        pParentMaterial = other.pParentMaterial;
        pendingTextures = std::move(other.pendingTextures);
        pendingBuffers = std::move(other.pendingBuffers);
        descriptorSet = other.descriptorSet;

        other.descriptorSet = VK_NULL_HANDLE;
        other.pContext = nullptr;
        other.pParentMaterial = nullptr;
    }
    return *this;
}

void MaterialInstance::bindTexture(const std::string& name, const AnvilTexture& inTexture)
{
    if (!pParentMaterial || !pParentMaterial->hasBinding(name))
    {
        std::cerr << "Binding not found for: " << name << std::endl;
        return;
    }

    pendingTextures.push_back({.name = name, .texture = &inTexture});
}

void MaterialInstance::bindUniformBuffer(const std::string& name, const GPUBuffer& inBuffer)
{
    if (!pParentMaterial || !pParentMaterial->hasBinding(name))
    {
        return;
    }
    pendingBuffers.push_back({.name = name, .buffer = &inBuffer});
}
