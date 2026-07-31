// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_PIPELINE_H
#define ANVIL_VK_PIPELINE_H

/**
 * @file AnvilPipeline.h
 * @brief Builder pattern abstraction for configuring and compiling Vulkan graphics pipelines.
 */

#include <vector>

#include <volk.h>

#include "AnvilVulkanDebug.h"

/**
 * @brief Simple container wrapping a compiled Vulkan graphics pipeline handle.
 * @note Maybe needs pipeline layouts too.
 */
struct AnvilPipeline
{
    /** Underlying Vulkan pipeline object handle. */
    VkPipeline pipeline = VK_NULL_HANDLE;
};

/**
 * @brief Builder class for configuring fixed-function graphics pipeline states.
 *
 * Implements a chainable interface to incrementally configure Vulkan pipeline stages,
 * vertex input layouts, blending, rasterization, and dynamic rendering attachment formats
 * before compiling them into an AnvilPipeline.
 */
class AnvilPipelineBuilder
{
public:
    /**
     * @brief Constructs a new pipeline builder with sensible default fixed-function states.
     */
    AnvilPipelineBuilder();

private:
    VkFormat colorAttachmentFormat = VK_FORMAT_UNDEFINED;
    VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    VkPipelineMultisampleStateCreateInfo multisampling{};
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    VkPipelineRenderingCreateInfo dynamicRendering{};

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkVertexInputBindingDescription> vertexBindings;
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;

public:
    /**
     * @brief Configures vertex input binding and attribute descriptions for the pipeline.
     * @param inBinding Vector describing the memory spacing and input rate of vertex buffers.
     * @param inAttributes Vector mapping vertex buffer offsets to shader input locations.
     * @return Reference to this builder for method chaining.
     */
    AnvilPipelineBuilder& setVertexInput(
        const std::vector<VkVertexInputBindingDescription>& inBinding,
        const std::vector<VkVertexInputAttributeDescription>& inAttributes
    );

    /**
     * @brief Sets the compiled vertex and fragment shader stages for the pipeline.
     * @param inVertexShader Vulkan shader module containing vertex shader SPIR-V bytecode.
     * @param inFragmentShader Vulkan shader module containing fragment shader SPIR-V bytecode.
     * @return Reference to this builder for method chaining.
     */
    AnvilPipelineBuilder& setShaders(VkShaderModule inVertexShader, VkShaderModule inFragmentShader);

    /**
     *
     * @param inColorFormat
     * @return
     */
    AnvilPipelineBuilder& setColorAttachmentFormat(VkFormat inColorFormat);

    /**
     *
     * @param inDepthFormat
     * @return
     */
    AnvilPipelineBuilder& setDepthAttachmentFormat(VkFormat inDepthFormat);

    /**
     *
     * @param bDepthWriteEnable
     * @param inCompareOp
     * @return
     */
    AnvilPipelineBuilder& enableDepthTest(bool bDepthWriteEnable, VkCompareOp inCompareOp);

    /**
     *
     * @param inTopology
     * @return
     */
    AnvilPipelineBuilder& setInputTopology(VkPrimitiveTopology inTopology);

    /**
     *
     * @param inPolygonMode
     * @return
     */
    AnvilPipelineBuilder& setPolygonMode(VkPolygonMode inPolygonMode);

    /**
     *
     * @param inCullMode
     * @param inFrontFace
     * @return
     */
    AnvilPipelineBuilder& setCullMode(VkCullModeFlags inCullMode, VkFrontFace inFrontFace);

    /**
     *
     * @return
     */
    AnvilPipelineBuilder& disableBlending();

    /**
     *
     * @param inDevice
     * @param inPipelineLayout
     * @param aDebugName
     * @param aDbgSrcLoc
     * @return
     */
    AnvilPipeline buildPipeline(const VkDevice& inDevice, const VkPipelineLayout& inPipelineLayout ANVIL_DEBUG_DECL()) const;
};

#endif //ANVIL_VK_PIPELINE_H
