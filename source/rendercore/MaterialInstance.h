// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_MATERIALINSTANCE_H
#define ANVIL_VK_MATERIALINSTANCE_H

/**
 * @file MaterialInstance.h
 * @brief Lightweight material instance that holds a specific descriptor set and resource bindings.
 */

#include <string>
#include <vector>
#include <volk.h>

#include "GPUBuffer.h"
#include "TextureLoader.h"

class AnvilMaterial;
class VulkanContext;

/**
 * @brief Represents a deferred texture binding operation.
 */
struct PendingTextureBind
{
    std::string name;
    const AnvilTexture* texture;
};

/**
 * @brief Represents a deferred uniform buffer binding operation.
 */
struct PendingBufferBind
{
    std::string name;
    const GPUBuffer* buffer;
};

/**
 * @brief A unique instance of an AnvilMaterial containing specific resource bindings.
 *
 * Spawned by an AnvilMaterial factory. Each instance owns a unique Vulkan descriptor set
 * allocated from the parent material's pool. It allows multiple objects to share the same
 * shader pipeline while using different textures or buffer data.
 *
 * @note This class in non-copyable. Moving is allowed.
 */
class MaterialInstance
{
    friend class AnvilMaterial;

public:
    MaterialInstance() = default;
    ~MaterialInstance() = default;

    MaterialInstance(const MaterialInstance&) = delete;
    MaterialInstance& operator=(const MaterialInstance&) = delete;

    MaterialInstance(MaterialInstance&&) noexcept;
    MaterialInstance& operator=(MaterialInstance&&) noexcept;

private:
    VulkanContext* pContext = nullptr;
    const AnvilMaterial* pParentMaterial = nullptr;

    std::vector<PendingTextureBind> pendingTextures;
    std::vector<PendingBufferBind> pendingBuffers;

    [[maybe_unused]] bool bDirty = false;

public:
    /** The Vulkan descriptor set unique to this instance. */
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

    /**
     * @brief Queues a texture to be bound to a shader sampler variable by name.
     *
     * @param name The variable name of the sampled texture in the Slang shader code.
     * @param inTexture Reference to the loaded AnvilTexture resource.
     *
     * @note Changes do not take effect on the GPU until updateDescriptorSets() is called.
     */
    void bindTexture(const std::string& name, const AnvilTexture& inTexture);

    /**
     * @brief Queues a uniform buffer to be bound to a shader uniform variable by name.
     *
     * @param name The variable name of uneform buffer in the Slang shader code.
     * @param inBuffer Reference to the GPU buffer containing the Uniform data.
     *
     * @note Changes do not take effect on the GPU until updateDescriptorSets() is called.
     */
    void bindUniformBuffer(const std::string& name, const GPUBuffer& inBuffer);

    /**
     * @brief Queues a storage buffer (SSBO) to be bound to a shader variable by name.
     *
     * @param name The variable name of the storage buffer in the Slang shader code.
     * @param inBuffer Reference to the GPU buffer containing the Uniform data.
     *
     * @note Changes do not take effect on the GPU until updateDescriptorSets() is called.
     */
    void bindStorageBuffer(const std::string& name, const GPUBuffer& inBuffer);

    /**
     * @brief Flushes all queued texture and buffer bindings to the GPU descriptor set.
     *
     * Calls vkUpdateDescriptorSets for all pending writes staged via bindTexture() or
     * bindUniformBuffer(), then clears the pending write queue.
     *
     * @note Only handles the following images and uniform buffers:
     * @code
     * VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
     * VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
     * VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
     * @endcode
     *
     * @see bindTexture
     * @see bindUniformBuffer
     */
    void updateDescriptorSets();
};


#endif //ANVIL_VK_MATERIALINSTANCE_H
