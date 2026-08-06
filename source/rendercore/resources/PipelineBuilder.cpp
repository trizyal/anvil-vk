// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "PipelineBuilder.h"

#include <stdexcept>

PipelineBuilder::PipelineBuilder()
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

    // Multisampling defaults
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
}

PipelineBuilder& PipelineBuilder::setVertexInput(const std::vector<VkVertexInputBindingDescription>& inBinding, const std::vector<VkVertexInputAttributeDescription>& inAttributes)
{
    vertexBindings = inBinding;
    vertexAttributes = inAttributes;

    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexBindings.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexBindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.data();

    return *this;
}

PipelineBuilder& PipelineBuilder::setShaders(VkShaderModule inVertexShader, VkShaderModule inFragmentShader)
{
    // TODO: If there are more stages, need to figure out how that will go
    shaderStages.clear();

    VkPipelineShaderStageCreateInfo vert_stage_info{};
    vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module = inVertexShader;
    vert_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_info{};
    frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module = inFragmentShader;
    frag_stage_info.pName = "main";

    shaderStages.push_back(vert_stage_info);
    shaderStages.push_back(frag_stage_info);

    return *this;
}

PipelineBuilder& PipelineBuilder::setColorAttachmentFormat(VkFormat inColorFormat)
{
    colorAttachmentFormat = inColorFormat;
    dynamicRendering.colorAttachmentCount = 1;
    dynamicRendering.pColorAttachmentFormats = &colorAttachmentFormat;

    return *this;
}

PipelineBuilder& PipelineBuilder::setDepthAttachmentFormat(VkFormat inDepthFormat)
{
    depthAttachmentFormat = inDepthFormat;

    // Hook it into the dynamic rendering struct
    dynamicRendering.depthAttachmentFormat = depthAttachmentFormat;

    return *this;
}

PipelineBuilder& PipelineBuilder::enableDepthTest(bool bDepthWriteEnable, VkCompareOp inCompareOp)
{
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = bDepthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = inCompareOp;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;

    return *this;
}

PipelineBuilder& PipelineBuilder::setInputTopology(VkPrimitiveTopology inTopology)
{
    inputAssembly.topology = inTopology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    return *this;
}

PipelineBuilder& PipelineBuilder::setPolygonMode(VkPolygonMode inPolygonMode)
{
    rasterizer.polygonMode = inPolygonMode;
    rasterizer.lineWidth = 1.0f;

    return *this;
}

PipelineBuilder& PipelineBuilder::setCullMode(VkCullModeFlags inCullMode, VkFrontFace inFrontFace)
{
    rasterizer.cullMode = inCullMode;
    rasterizer.frontFace = inFrontFace;

    return *this;
}

PipelineBuilder& PipelineBuilder::disableBlending()
{
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    return *this;
}

AnvilPipeline PipelineBuilder::buildPipeline(const VkDevice& inDevice, const VkPipelineLayout& inPipelineLayout ANVIL_DEBUG_DEFN) const
{
    // Viewport state setup
    // Using dynamic states so we can resize the window
    VkPipelineViewportStateCreateInfo viewport_state_info{};
    viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_info.viewportCount = 1;
    viewport_state_info.scissorCount = 1;

    VkPipelineColorBlendStateCreateInfo color_blending_info{};
    color_blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_info.logicOpEnable = VK_FALSE;
    color_blending_info.attachmentCount = 1;
    color_blending_info.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state_info{};
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_info.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.pNext = &dynamicRendering;
    pipeline_create_info.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipeline_create_info.pStages = shaderStages.data();
    pipeline_create_info.pVertexInputState = &vertexInputInfo;
    pipeline_create_info.pInputAssemblyState = &inputAssembly;
    pipeline_create_info.pViewportState = &viewport_state_info; // Hard coded
    pipeline_create_info.pRasterizationState = &rasterizer;
    pipeline_create_info.pMultisampleState = &multisampling;
    pipeline_create_info.pDepthStencilState = &depthStencil;
    pipeline_create_info.pColorBlendState = &color_blending_info; // Hard coded
    pipeline_create_info.pDynamicState = &dynamic_state_info; // Hard coded
    pipeline_create_info.layout = inPipelineLayout;

    AnvilPipeline returnAnvilPipeline{};

    if (vkCreateGraphicsPipelines(inDevice, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &returnAnvilPipeline.pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    ANVIL_DEBUG_NAME(inDevice, returnAnvilPipeline.pipeline, VK_OBJECT_TYPE_PIPELINE);

    return returnAnvilPipeline;
}
