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
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    VkFormat colorAttachmentFormat;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    VkPipelineRenderingCreateInfo dynamicRendering;

public:
    AnvilPipelineBuilder();

    // Chainable methods to set state
    AnvilPipelineBuilder& setShaders(VkShaderModule inVertexShader, VkShaderModule inFragmentShader);
    AnvilPipelineBuilder& setColorAttachmentFormat(VkFormat inColorFormat);
    AnvilPipelineBuilder& setInputTopology(VkPrimitiveTopology inTopology);
    AnvilPipelineBuilder& setPolygonMode(VkPolygonMode inPolygonMode);
    AnvilPipelineBuilder& setCullMode(VkCullModeFlags inCullMode, VkFrontFace inFrontFace);
    AnvilPipelineBuilder& disableBlending();

    AnvilPipeline build(const VkDevice& inDevice, const VkPipelineLayout& inPipelineLayout);
};

#endif //ANVIL_VK_PIPELINE_H
