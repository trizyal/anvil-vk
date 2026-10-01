// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "RenderGraph.h"

#include "GPUTexture.h"

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
}
