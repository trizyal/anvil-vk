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

    // Prevent binding cross contamination
    if (pParentMaterial->getBinding(name).setIndex != setIndex)
    {
        throw std::runtime_error("MaterialInstance Error: '" + name + "' belongs to Set "
            + std::to_string(pParentMaterial->getBinding(name).setIndex)
            + " but this instance is managing Set " + std::to_string(setIndex));
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

void MaterialInstance::bindStorageBuffer(const std::string& name, const GPUBuffer& inBuffer)
{
    if (!pParentMaterial || !pParentMaterial->hasBinding(name))
    {
        return;
    }
    pendingBuffers.push_back({.name = name, .buffer = &inBuffer});
}

void MaterialInstance::updateDescriptorSets()
{
    if (pendingTextures.empty() && pendingBuffers.empty())
    {
        std::cerr << "No textures and buffers to bind." << std::endl;
        return;
    }

    std::vector<VkDescriptorImageInfo> image_infos_vector;
    image_infos_vector.reserve(pendingTextures.size());

    std::vector<VkDescriptorBufferInfo> buffer_infos_vector;
    buffer_infos_vector.reserve(pendingBuffers.size());

    std::vector<VkWriteDescriptorSet> writes_vector;
    writes_vector.reserve(pendingTextures.size() + pendingBuffers.size());

    for (const auto& [name, texture] : pendingTextures)
    {
        const ShaderBinding shader_binding = pParentMaterial->getBinding(name);

        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = texture->imageView;
        image_info.sampler = texture->sampler;
        image_infos_vector.push_back(image_info);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSet;
        write.dstBinding = shader_binding.bindingIndex;
        write.dstArrayElement = 0;
        write.descriptorType = shader_binding.descriptorType;
        write.descriptorCount = 1; //?
        writes_vector.push_back(write);
    }

    for (const auto& [name, gpu_buffer] : pendingBuffers)
    {
        const ShaderBinding shader_binding = pParentMaterial->getBinding(name);

        VkDescriptorBufferInfo info{};
        info.buffer = gpu_buffer->buffer;
        info.offset = 0;
        info.range = VK_WHOLE_SIZE;
        buffer_infos_vector.push_back(info);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSet;
        write.dstBinding = shader_binding.bindingIndex;
        write.dstArrayElement = 0;
        write.descriptorType = shader_binding.descriptorType;
        write.descriptorCount = 1; //?
        writes_vector.push_back(write);
    }

    size_t image_index = 0;
    size_t buffer_index = 0;
    for (auto& write : writes_vector)
    {
        if (write.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
            write.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
        {
            write.pImageInfo = &image_infos_vector[image_index];
            image_index++;
        }
        else if (write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
            write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        {
            write.pBufferInfo = &buffer_infos_vector[buffer_index];
            buffer_index++;
        }
    }

    vkUpdateDescriptorSets(pContext->device, static_cast<uint32_t>(writes_vector.size()), writes_vector.data(), 0, nullptr);

    pendingTextures.clear();
    pendingBuffers.clear();
}
