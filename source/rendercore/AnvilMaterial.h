// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_MATERIAL_H
#define ANVIL_VK_MATERIAL_H

/**
 * @file AnvilMaterial.h
 * @brief Material abstraction managing Slang shader compilation, reflection, and Vulkan descriptor binding.
 */

#include <string>

#include <volk.h>
#include <slang.h>
#include <slang-com-ptr.h>

#include "GPUBuffer.h"
#include "ShaderCompiler.h"
#include "ShaderModule.h"
#include "TextureLoader.h"
#include "VulkanContext.h"

/**
 * @brief Cached Vulkan descriptor binding metadata extracted via Slang shader reflection.
 */
struct ShaderBinding
{
    /** Descriptor set index (e.g., set = 0) */
    uint32_t set;

    /** Binding slot index within the descriptor set. */
    uint32_t binding;

    /** Vulkan resource type (e.g., VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER). */
    VkDescriptorType descriptorType;
};

/**
 * @brief Encapsulates shaders, pipeline layout, and GPU resource bindings for a renderable surface.
 *
 * Owns its vertex and fragment shader modules, Uses Slang reflection metadata during build time
 * to automatically generate Vulkan descriptor set layouts, pipeline layouts, and descriptor pools.
 * Provides a string-based binding interface that batches descriptor updates on the CPU before flushing to the GPU.
 *
 * @note This class in non-copyable and non-movable. May need to change that.
 *
 * @warning Only stores one pair of vertex and fragment shaders, may need re-architecting.
 */
class AnvilMaterial
{
public:
    AnvilMaterial() = default;
    ~AnvilMaterial() = default;

    AnvilMaterial(const AnvilMaterial&) = delete;
    AnvilMaterial& operator=(const AnvilMaterial&) = delete;
    AnvilMaterial(AnvilMaterial&&) = delete;
    AnvilMaterial& operator=(AnvilMaterial&&) = delete;

    /** Owned vertex shader module and associated SPIR-V bytecode. */
    ShaderModule vertexShader;

    /** Owned fragment shader module and associated SPIR-V bytecode. */
    ShaderModule fragmentShader;

    /** Reflected layout for the material's descriptor set. */
    VkDescriptorSetLayout materialDescriptorSetLayout = VK_NULL_HANDLE;

    /** Pool allocated specifically for this material's descriptor sets. */
    VkDescriptorPool materialDescriptorPool = VK_NULL_HANDLE;

    /** Allocated descriptor set instance for binding resources. */
    VkDescriptorSet materialDescriptorSet = VK_NULL_HANDLE;

    /** Layout describing descriptor sets and push constants for this material. */
    VkPipelineLayout materialPipelineLayout = VK_NULL_HANDLE;

    /** Bitmask of pipeline stages utilizing push constants in this material. */
    VkShaderStageFlags pushConstantStages = 0;

private:
    VulkanContext* ptrAContext = nullptr;

    std::unordered_map<std::string, ShaderBinding> bindingMap;
    std::vector<VkWriteDescriptorSet> pendingWrites;
    std::vector<VkDescriptorImageInfo> pendingImageInfos;
    std::vector<VkDescriptorBufferInfo> pendingBufferInfos;

public:
    /**
     * @brief Compiles shaders, reflects resource layouts, and allocates Vulkan descriptor objects.
     *
     * Compiles the requested vertex and fragment via Slang, inspects the resulting reflection
     * metadata to build descriptor layouts and push constants, and allocates a descriptor set.
     *
     * @param inContext Reference to the active Anvil Vulkan context.
     * @param inCompiler Reference to the active Slang shader compiler.
     * @param inVertReq Compilation request parameters for the vertex shader stage.
     * @param inFragReq Compilation request parameters for the fragment shader stage.
     *
     * @throws std::runtime_error If shader compilation fails or Vulkan layouts cannot be created.
     */
    void buildMaterial(VulkanContext& inContext,
                       ShaderCompiler& inCompiler,
                       const AnvilShaders::ShaderCompileRequest& inVertReq,
                       const AnvilShaders::ShaderCompileRequest& inFragReq);

    /**
     * @brief Destroys all Vulkan layouts, descriptor pools, and shader modules owned by this material.
     */
    void destroyMaterial() const;

    /**
     * @brief Queues a texture to be bound to a shader sampler variable by name.
     *
     * Looks up the shader binding slot from reflection metadata and stages a descriptor write.
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
     * Looks up the shader binding slot from reflection metadata and stages a descriptor write.
     *
     * @param name The variable name of the sampled texture in the Slang shader code.
     * @param inBuffer Reference to the GPU buffer containing the Uniform data.
     *
     * @note Changes do not take effect on the GPU until updateDescriptorSets() is called.
     */
    void bindUniformBuffer(const std::string& name, const GPUBuffer& inBuffer);

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

private:
    /**
     * @brief Inspects Slang reflection metadata to populate descriptor layouts and push constant ranges.
     *
     * @param[in] linkedProgram Slang component type containing layout reflection data.
     * @param[in] stage Target Vulkan shader stage bitmask for these bindings.
     * @param[in,out] outLayoutBindings Output vector appended with reflected descriptor set binding layouts.
     * @param[in,out] outPushConstants Output vector appended with reflected push constant ranges.
     */
    void reflectShader(slang::IComponentType* linkedProgram,
        VkShaderStageFlagBits stage,
        std::vector<VkDescriptorSetLayoutBinding>& outLayoutBindings,
        std::vector<VkPushConstantRange>& outPushConstants);
};

#endif //ANVIL_VK_MATERIAL_H
