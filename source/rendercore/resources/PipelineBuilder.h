// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_PIPELINEBUILDER_H
#define ANVIL_VK_PIPELINEBUILDER_H

/**
 * @file PipelineBuilder.h
 * @brief Builder pattern abstraction for configuring and compiling Vulkan graphics pipelines.
 */

#include <vector>

#include <volk.h>

#include "VulkanDebug.h"

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
class PipelineBuilder
{
public:
    /**
     * @brief Constructs a new pipeline builder with sensible default fixed-function states.
     */
    PipelineBuilder();

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

    /** Shader stages included in the graphics pipeline. */
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    /** Vertex buffer binding descriptions used by the vertex input state. */
    std::vector<VkVertexInputBindingDescription> vertexBindings;

    /** Vertex attribute descriptions used by the vertex input state. */
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;

public:
    /**
     * @brief Configures vertex input binding and attribute descriptions for the pipeline.
     * @param inBinding Vector describing the memory spacing and input rate of vertex buffers.
     * @param inAttributes Vector mapping vertex buffer offsets to shader input locations.
     * @return Reference to this builder for method chaining.
     */
    PipelineBuilder& setVertexInput(
        const std::vector<VkVertexInputBindingDescription>& inBinding,
        const std::vector<VkVertexInputAttributeDescription>& inAttributes
    );

    /**
     * @brief Sets the compiled vertex and fragment shader stages for the pipeline.
     * @param inVertexShader Vulkan shader module containing vertex shader SPIR-V bytecode.
     * @param inFragmentShader Vulkan shader module containing fragment shader SPIR-V bytecode.
     * @return Reference to this builder for method chaining.
     */
    PipelineBuilder& setShaders(VkShaderModule inVertexShader, VkShaderModule inFragmentShader);

    /**
     * @brief Sets the format of the color attachment used by dynamic rendering.
     * @param inColorFormat Vulkan format of the color attachment.
     * @return Reference to this builder for method chaining.
     */
    PipelineBuilder& setColorAttachmentFormat(VkFormat inColorFormat);

    /**
     * @brief Sets the format of the depth attachment used by dynamic rendering.
     * @param inDepthFormat Vulkan format of the depth attachment.
     * @return Reference to this builder for method chaining.
     */
    PipelineBuilder& setDepthAttachmentFormat(VkFormat inDepthFormat);

    /**
     * @brief Enables depth testing and configures depth writes and comparison.
     * @param bDepthWriteEnable 'true' if depth write should be enabled, `false` otherwise.
     * @param inCompareOp Comparison operation used by the depth test (e.g. VK_COMPARE_OP_LESS).
     * @return Reference to this builder for method chaining.
     */
    PipelineBuilder& enableDepthTest(bool bDepthWriteEnable, VkCompareOp inCompareOp);

    /**
     * @brief Sets the primitive topology used by the input assembly stage.
     * @param inTopology Primitive topology used to interpret vertex data (e.g. VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST).
     * @return Reference to this builder for method chaining.
     */
    PipelineBuilder& setInputTopology(VkPrimitiveTopology inTopology);

    /**
     * @brief Sets the polygon rasterization mode.
     * @param inPolygonMode Polygon rasterization stage (e.g. VK_POLYGON_MODE_FILL).
     * @return Reference to this builder for method chaining.
     */
    PipelineBuilder& setPolygonMode(VkPolygonMode inPolygonMode);

    /**
     *
     * @param inCullMode Faces to cull during rasterization (e.g. VK_CULL_MODE_BACK_BIT).
     * @param inFrontFace Winding order used to determine front-facing primitives (e.g. VK_FRONT_FACE_COUNTER_CLOCKWISE).
     * @return Reference to this builder for method chaining.
     */
    PipelineBuilder& setCullMode(VkCullModeFlags inCullMode, VkFrontFace inFrontFace);

    /**
     * @brief Disables color blending for the pipeline's color attachment.
     *
     * Configures the color blend attachment state so that fragment shader output
     * is written directly to the color attachment without blending.
     * @return Reference to this builder for method chaining.
     */
    PipelineBuilder& disableBlending();

    /**
     * @brief Builds a Vulkan graphics pipeline using the configured state.
     * @param inDevice Vulkan logical device used to create the pipeline.
     * @param inPipelineLayout Pipeline layout describing the resources accessible to the pipeline's shaders.
     * @param aDebugName Optional debug name for Vulkan object.
     * @param aDbgSrcLoc Automatic.
     * @return AnvilPipeline containing the created Vulkan graphics pipeline handle.
     *
     * @see AnvilPipeline
     */
    AnvilPipeline buildPipeline(const VkDevice& inDevice, const VkPipelineLayout& inPipelineLayout ANVIL_DEBUG_DECL()) const;
};

#endif //ANVIL_VK_PIPELINEBUILDER_H
