// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "RenderGraph.h"

#include <tracy/Tracy.hpp>

#include "GPUTexture.h"
#include "Logger.h"
#include "Swapchain.h"
#include "Trace.h"
#include "VulkanStrings.h"
#include "VulkanUtilities.h"

// RenderPassBuilder

RenderPassBuilder& RenderPassBuilder::read(const GPUTexture& tex)
{
    GraphAttachment read_attachment;
    read_attachment.image = tex.image;
    read_attachment.imageView = tex.imageView;

    read_attachment.format = tex.format;
    read_attachment.extent = {tex.width, tex.height};

    read_attachment.currentLayout = &tex.currentLayout;
    read_attachment.loadOp = LoadOp::DontCare;
    read_attachment.clearValue = {};
    read_attachment.isDepth = (tex.format == Format::D32_SFLOAT);

    node.reads.push_back(std::move(read_attachment));

    return *this;
}

RenderPassBuilder& RenderPassBuilder::writeColor(const GPUTexture& tex, LoadOp loadOp, glm::vec4 clearColor)
{
    VkClearValue clearValue;
    clearValue.color = {{clearColor.r, clearColor.g, clearColor.b, clearColor.a}};

    GraphAttachment color_write_attachment;
    color_write_attachment.image = tex.image;
    color_write_attachment.imageView = tex.imageView;
    color_write_attachment.format = tex.format;

    color_write_attachment.extent = {tex.width, tex.height};
    color_write_attachment.currentLayout = &tex.currentLayout;

    color_write_attachment.loadOp = loadOp;
    color_write_attachment.clearValue = clearValue;
    color_write_attachment.isDepth = false;

    node.colorWrites.push_back(std::move(color_write_attachment));

    return *this;
}

RenderPassBuilder& RenderPassBuilder::writeDepth(const GPUTexture& tex, LoadOp loadOp, float clearDepth)
{
    VkClearValue clearValue;
    clearValue.depthStencil = {.depth = clearDepth, .stencil = 0};

    GraphAttachment depth_write_attachment;
    depth_write_attachment.image = tex.image;
    depth_write_attachment.imageView = tex.imageView;
    depth_write_attachment.format = tex.format;

    depth_write_attachment.extent = {tex.width, tex.height};
    depth_write_attachment.currentLayout = &tex.currentLayout;

    depth_write_attachment.loadOp = loadOp;
    depth_write_attachment.clearValue = clearValue;
    depth_write_attachment.isDepth = true;

    node.colorWrites.push_back(std::move(depth_write_attachment));

    return *this;
}

RenderPassBuilder& RenderPassBuilder::writeSwapchain(Swapchain& swapchain, uint32_t imageIndex, LoadOp loadOp, glm::vec4 clearColor)
{
    VkClearValue clearValue;
    clearValue.color = {{clearColor.r, clearColor.g, clearColor.b, clearColor.a}};

    GraphAttachment swapchain_attachment;
    swapchain_attachment.image = swapchain.swapchainImages[imageIndex];
    swapchain_attachment.imageView = swapchain.swapchainImageViews[imageIndex];
    swapchain_attachment.format = swapchain.swapchainFormat;

    swapchain_attachment.extent = swapchain.swapchainExtent;
    swapchain_attachment.currentLayout = &swapchain.swapchainImageLayouts[imageIndex];

    swapchain_attachment.loadOp = loadOp;
    swapchain_attachment.clearValue = clearValue;
    swapchain_attachment.isDepth = false;

    node.colorWrites.push_back(std::move(swapchain_attachment));

    return *this;
}

void RenderPassBuilder::execute(std::function<void(VkCommandBuffer)> callback) const
{
    node.executeCallback = std::move(callback);
}


// RenderGraph

RenderPassBuilder RenderGraph::addPass(const std::string& name)
{
    GraphPassNode node;
    node.name = name;
    node.reads = {};
    node.colorWrites = {};
    node.depthWrite = std::nullopt;
    node.executeCallback = nullptr;

    passes.push_back(std::move(node));
    return RenderPassBuilder(passes.back());
}

void RenderGraph::execute(VkCommandBuffer cmd)
{
    SCOPE_CPU;
    for (GraphPassNode& pass : passes)
    {
        SCOPE_CPU_NAME("Should say the pass.name");
        for (const GraphAttachment& read : pass.reads)
        {
            VulkanUtils::TransitionImage(cmd, read.image, *read.currentLayout, ImageLayout::ShaderReadOnly, read.isDepth);
        }

        std::vector<VkRenderingAttachmentInfo> color_attachments;
        VkExtent2D render_extent = {0, 0};

        for (GraphAttachment& write : pass.colorWrites)
        {
            VulkanUtils::TransitionImage(cmd, write.image, *write.currentLayout, ImageLayout::ColorAttachment, false);

            VkRenderingAttachmentInfo color_info{};
            color_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            color_info.imageView = write.imageView;
            color_info.imageLayout = vk(ImageLayout::ColorAttachment);
            color_info.loadOp = vk(write.loadOp);
            color_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            color_info.clearValue = write.clearValue;
            color_attachments.push_back(color_info);

            if (render_extent.width == 0)
            {
                render_extent = write.extent;
            }
        }

        VkRenderingAttachmentInfo depth_info{};
        depth_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        if (pass.depthWrite)
        {
            VulkanUtils::TransitionImage(cmd, pass.depthWrite->image, *pass.depthWrite->currentLayout, ImageLayout::DepthAttachment, true);

            depth_info.imageView = pass.depthWrite->imageView;
            depth_info.imageLayout = vk(ImageLayout::DepthAttachment);
            depth_info.loadOp = vk(pass.depthWrite->loadOp);
            depth_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depth_info.clearValue = pass.depthWrite->clearValue;

            if (render_extent.width == 0)
            {
                render_extent = pass.depthWrite->extent;
            }
        }

        VkRenderingInfo render_info{};
        render_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        render_info.renderArea = {{0, 0}, render_extent};
        render_info.layerCount = 1;
        render_info.colorAttachmentCount = static_cast<uint32_t>(color_attachments.size());
        render_info.pColorAttachments = color_attachments.data();
        if (pass.depthWrite)
        {
            render_info.pDepthAttachment = &depth_info;
        }

        vkCmdBeginRendering(cmd, &render_info);

        if (pass.executeCallback)
        {
            pass.executeCallback(cmd);
        }

        vkCmdEndRendering(cmd);
    }
}
