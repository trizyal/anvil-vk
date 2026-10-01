// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

/**
 * @file RenderGraph.h
 * @brief Declarative Render Graph builder and dynamic rendering pass orchestrator.
 */

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <volk.h>

class Swapchain;
class GPUTexture;

/**
 * @brief Concise wrapper for commonly used Vulkan load operations.
 */
enum class LoadOp : uint32_t
{
    Clear    = VK_ATTACHMENT_LOAD_OP_CLEAR,
    Load     = VK_ATTACHMENT_LOAD_OP_LOAD,
    DontCare = VK_ATTACHMENT_LOAD_OP_DONT_CARE
};

/**
 * @brief Helper function to quickly cast the ImageLayout enum back to a VkImageLayout.
 */
inline VkAttachmentLoadOp vk(LoadOp o)
{
    return static_cast<VkAttachmentLoadOp>(o);
}

/**
 * @brief Concise wrapper for commonly used Vulkan image layouts.
 */
enum class ImageLayout : uint32_t
{
    Undefined       = VK_IMAGE_LAYOUT_UNDEFINED,
    ColorAttachment = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    DepthAttachment = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
    ShaderReadOnly  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    Present         = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    TransferSrc     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    TransferDst     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
};

/**
 * @brief Helper function to quickly cast the ImageLayout enum back to a VkImageLayout.
 */
inline VkImageLayout vk(ImageLayout l)
{
    return static_cast<VkImageLayout>(l);
}

/**
 * @brief Concise wrapper for commonly used Vulkan image and buffer formats.
 */
enum class Format : uint32_t
{
    Undefined           = VK_FORMAT_UNDEFINED,

    // --(Standard Color / Textures)--
    // 8-bit 4-channels unsigned normalised integers (0-255)
    RGBA8_UNORM      = VK_FORMAT_R8G8B8A8_UNORM,

    // 8-bit 4-channels unsigned integers (0-255)
    RGBA8_SRGB       = VK_FORMAT_R8G8B8A8_SRGB,

    // (HDR / G-Buffer Normals & World Pos)
    // 16-bit 4-channels signed floats
    RGBA16_SFLOAT       = VK_FORMAT_R16G16B16A16_SFLOAT,

    // --(Vertex attributes 32-bits)--
    // 32-bit 2-channels signed floats
    RG32_SFLOAT       = VK_FORMAT_R32G32_SFLOAT,

    // 32-bit 3-channels signed floats
    RGB32_SFLOAT    = VK_FORMAT_R32G32B32_SFLOAT,

    // 32-bit 4-channels signed floats
    RGBA32_SFLOAT = VK_FORMAT_R32G32B32A32_SFLOAT,

    // 32-bit 4-channels unsigned integers
    RGBA32_UINT   = VK_FORMAT_R32G32B32A32_UINT,

    // --(Depth Stencil)--
    // 32-bit 1-channel signed float
    D32_SFLOAT          = VK_FORMAT_D32_SFLOAT
};

/**
 * @brief Helper function to quickly cast the Format enum back to a VkFormat.
 */
inline VkFormat vk(Format f) {
    return static_cast<VkFormat>(f);
}

/**
 * @brief Represents a single GPU image attachment declared within a render pass.
 *
 * Encapsulates the Vulkan image handles, dynamic load operations, clear values,
 * and a pointer to the texture's active layout state variable for automatic synchronization.
 */
struct GraphAttachment
{
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    Format format = Format::Undefined;
    VkExtent2D extent{0, 0};
    ImageLayout* currentLayout = nullptr;
    LoadOp loadOp = LoadOp::DontCare;

    /** Color or depth/stencil clear values applied if loadOp is CLEAR. */
    VkClearValue clearValue{};

    /** True if this attachment is used as a depth/stencil target. */
    bool isDepth = false;
};

/**
 * @brief Internal node container holding state, dependencies, and execution logic for a single pass.
 */
struct GraphPassNode
{
    /** Debug/profiling name assigned to this pass. */
    std::string name;

    /** Textures sampled in the shader stages during this pass. */
    std::vector<GraphAttachment> reads;

    /** Color render targets written to during dynamic rendering. */
    std::vector<GraphAttachment> colorWrites;

    /** Optional depth attachment written to during dynamic rendering. */
    std::optional<GraphAttachment> depthWrite;

    /** User callback recording actual draw commands. */
    std::function<void(VkCommandBuffer)> executeCallback;
};

/**
 * @brief Builder interface providing an API to declare render pass dependencies and logic.
 */
class RenderPassBuilder
{
public:
    RenderPassBuilder() = delete;
    ~RenderPassBuilder() = default;

private:
    GraphPassNode& node;

public:
    /**
     * @brief Constructs a pass builder referencing a target pass node.
     * @param n Reference to the pass node being configured.
     */
    explicit RenderPassBuilder(GraphPassNode& n)
    : node(n) {}

    /**
     * @brief Declares a texture dependency that will be sampled/read during this pass.
     * @param tex Texture resource to transition into VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
     * @return Reference to this builder for method chaining.
     */
    RenderPassBuilder& read(const GPUTexture& tex);

    /**
     * @brief Declares a GPUTexture as a color render attachment target for this pass.
     * @param tex Destination GPU texture attachment.
     * @param loadOp Operation performed on the attachment at pass start (Clear, Load, DontCare).
     * @param clearColor RGBA clear color applied if loadOp is Clear.
     * @return Reference to this builder for method chaining.
     */
    RenderPassBuilder& writeColor(const GPUTexture& tex, LoadOp loadOp, glm::vec4 clearColor = glm::vec4(0.0f));

    /**
     * @brief Declares a GPUTexture as a depth attachment target for this pass.
     * @param tex Destination depth GPU texture.
     * @param loadOp Operation performed on the depth attachment at pass start (Clear, Load, DontCare).
     * @param clearDepth Depth clear value applied if loadOp is Clear (defaults to 1.0f).
     * @return Reference to this builder for method chaining.
     */
    RenderPassBuilder& writeDepth(const GPUTexture& tex, LoadOp loadOp, float clearDepth = 1.0f);

    /**
     * @brief Helper method to declare a active swapchain presentation image as a color render target.
     * @param swapchain Reference to the active application Swapchain instance.
     * @param imageIndex Current frame's swapchain presentation image index.
     * @param loadOp Operation performed on the swapchain attachment at pass start.
     * @param clearColor RGBA clear color applied if loadOp is VK_ATTACHMENT_LOAD_OP_CLEAR.
     * @return Reference to this builder for method chaining.
     */
    RenderPassBuilder& writeSwapchain(Swapchain& swapchain, uint32_t imageIndex, LoadOp loadOp, glm::vec4 clearColor = glm::vec4(0.0f));

    /**
     * @brief Assigns the application execution callback containing command buffer draw logic.
     * @param callback Lambda or function receiving an active VkCommandBuffer to record draw calls.
     */
    void execute(std::function<void(VkCommandBuffer)> callback);
};

/**
 * @brief High-level Frame Graph managing dynamic rendering pass execution and image barriers.
 *
 * Compiles declared pass nodes, emits required pipeline image layout transition barriers,
 * opens Vulkan 1.3 dynamic rendering blocks, and executes recorded application draw callbacks.
 */
class RenderGraph
{
    using enum ImageLayout;
    using enum LoadOp;

public:
    RenderGraph() = default;
    ~RenderGraph() = default;

private:
    std::vector<GraphPassNode> passes;

public:
    /**
     * @brief Creates and registers a new pass node in the graph sequence.
     * @param name Descriptive debug label for the pass.
     * @return A RenderPassBuilder instance to configure pass inputs, outputs, and execution callbacks.
     */
    RenderPassBuilder addPass(const std::string& name);

    /**
     * @brief Compiles barriers, sets up dynamic rendering attachments, and executes all declared passes.
     * @param cmd Active primary command buffer recorded during the frame.
     */
    void execute(VkCommandBuffer cmd);

private:
    /**
     * @brief Evaluates current and required layouts and inserts a VkImageMemoryBarrier if a transition is needed.
     * @param cmd Active Vulkan command buffer.
     * @param image Target Vulkan image handle.
     * @param currentLayout Reference to the tracked layout state variable (updated upon barrier emission).
     * @param newLayout Target ImageLayout required for the upcoming operation.
     * @param isDepth True if evaluating a depth aspect image transition.
     */
    void transitionImage(VkCommandBuffer cmd, VkImage image, ImageLayout& currentLayout, ImageLayout newLayout, bool isDepth);
};

