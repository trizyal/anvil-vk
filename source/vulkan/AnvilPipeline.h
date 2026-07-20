// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_PIPELINE_H
#define ANVIL_VK_PIPELINE_H

#include <vector>

#include <volk.h>

struct AnvilPipeline
{
    VkPipeline pipeline;
    VkPipelineLayout pipelinelayout;
};

class AnvilPipelineBuilder
{
private:
    VkFormat colorAttachmentFormat = VK_FORMAT_UNDEFINED;
    VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    VkPipelineRenderingCreateInfo dynamicRendering;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkVertexInputBindingDescription> vertexBindings;
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;

public:
    AnvilPipelineBuilder();

    // Chainable methods to set state
    AnvilPipelineBuilder& setVertexInput(
        const std::vector<VkVertexInputBindingDescription>& inBinding,
        const std::vector<VkVertexInputAttributeDescription>& inAttributes
    );
    AnvilPipelineBuilder& setShaders(VkShaderModule inVertexShader, VkShaderModule inFragmentShader);
    AnvilPipelineBuilder& setColorAttachmentFormat(VkFormat inColorFormat);
    AnvilPipelineBuilder& setDepthAttachmentFormat(VkFormat inDepthFormat);
    AnvilPipelineBuilder& enableDepthTest(bool bDepthWriteEnable, VkCompareOp inCompareOp);

    AnvilPipelineBuilder& setInputTopology(VkPrimitiveTopology inTopology);
    AnvilPipelineBuilder& setPolygonMode(VkPolygonMode inPolygonMode);
    AnvilPipelineBuilder& setCullMode(VkCullModeFlags inCullMode, VkFrontFace inFrontFace);
    AnvilPipelineBuilder& disableBlending();

    AnvilPipeline build(const VkDevice& inDevice, const VkPipelineLayout& inPipelineLayout);
};

#endif //ANVIL_VK_PIPELINE_H
