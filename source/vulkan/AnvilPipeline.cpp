// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilPipeline.h"

#include <stdexcept>

AnvilPipelineBuilder::AnvilPipelineBuilder() : colorAttachmentFormat()
{
    // Initialise standard structs to safe zero values
    vertexInputInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    inputAssembly = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    rasterizer = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    colorBlendAttachment = {};
    multisampling = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    depthStencil = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    // dynamicRendering = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicRendering = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
}

AnvilPipelineBuilder& AnvilPipelineBuilder::setVertexInput(const std::vector<VkVertexInputBindingDescription>& inBinding, const std::vector<VkVertexInputAttributeDescription>& inAttributes)
{
    vertexBindings = inBinding;
    vertexAttributes = inAttributes;

    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexBindings.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexBindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();

    return *this;
}

AnvilPipelineBuilder& AnvilPipelineBuilder::setShaders(VkShaderModule inVertexShader, VkShaderModule inFragmentShader)
{
    // TODO: If there are more stages, need to figure out how that will go
    shaderStages.clear();

    VkPipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = inVertexShader;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = inFragmentShader;
    fragStageInfo.pName = "main";

    shaderStages.push_back(vertStageInfo);
    shaderStages.push_back(fragStageInfo);

    return *this;
}

AnvilPipelineBuilder& AnvilPipelineBuilder::setColorAttachmentFormat(VkFormat inColorFormat)
{
    colorAttachmentFormat = inColorFormat;
    dynamicRendering.colorAttachmentCount = 1;
    dynamicRendering.pColorAttachmentFormats = &colorAttachmentFormat;

    return *this;
}

AnvilPipelineBuilder& AnvilPipelineBuilder::setInputTopology(VkPrimitiveTopology inTopology)
{
    inputAssembly.topology = inTopology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    return *this;
}

AnvilPipelineBuilder& AnvilPipelineBuilder::setPolygonMode(VkPolygonMode inPolygonMode)
{
    rasterizer.polygonMode = inPolygonMode;
    rasterizer.lineWidth = 1.0f;

    return *this;
}

AnvilPipelineBuilder& AnvilPipelineBuilder::setCullMode(VkCullModeFlags inCullMode, VkFrontFace inFrontFace)
{
    rasterizer.cullMode = inCullMode;
    rasterizer.frontFace = inFrontFace;

    return *this;
}

AnvilPipelineBuilder& AnvilPipelineBuilder::disableBlending()
{
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    return *this;
}

AnvilPipeline AnvilPipelineBuilder::build(const VkDevice& inDevice, const VkPipelineLayout& inPipelineLayout)
{
    // Viewport state setup
    // Using dynamic states so we can resize the window
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    // Multisampling defaults
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.pNext = &dynamicRendering;
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pRasterizationState = &rasterizer;
    pipelineCreateInfo.pMultisampleState = &multisampling;
    pipelineCreateInfo.pDepthStencilState = &depthStencil;
    pipelineCreateInfo.pColorBlendState = &colorBlending;
    pipelineCreateInfo.pDynamicState = &dynamicStateInfo;
    pipelineCreateInfo.layout = inPipelineLayout;

    AnvilPipeline returnAnvilPipeline{};
    returnAnvilPipeline.pipelinelayout = inPipelineLayout;

    if (vkCreateGraphicsPipelines(inDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &returnAnvilPipeline.pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    return returnAnvilPipeline;
}
