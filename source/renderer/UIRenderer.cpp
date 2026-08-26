// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "UIRenderer.h"

#include <algorithm>
#include <stdexcept>

#include <volk.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "VulkanConfig.h"
#include "Swapchain.h"
#include "VulkanContext.h"
#include "DebugNames.h"
#include "UIElements.h"
#include "VulkanResult.h"

namespace
{
    VkDescriptorPoolSize ImGuiPoolSizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
}

bool UIRenderer::initializeUIRenderer(VulkanContext* inContext, GLFWwindow* inWindow, Swapchain* inSwapchain)
{
    pContext = inContext;

    VkDevice device = inContext->device;

    createDescriptorPool(device DNAME("ImGuiDescriptorPool"));

    // Initialize ImGui Core
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Enable Docking and Multi-Viewport (dragging windows outside the app)
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // 3. Initialize GLFW and Vulkan Backends
    ImGui_ImplGlfw_InitForVulkan(inWindow, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = AnvilVulkan::API_VERSION;
    init_info.Instance = inContext->instance;
    init_info.PhysicalDevice = inContext->physicalDevice;
    init_info.Device = device;
    init_info.QueueFamily = inContext->graphicsQueueIndex;
    init_info.Queue = inContext->graphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 3;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    // --- DYNAMIC RENDERING HOOKS ---
    init_info.UseDynamicRendering = true;

    // CRITICAL: We must store the format in a static variable (or a class member)
    // because ImGui's Viewport system will read this pointer LATER when you drag a window!
    colorFormat = inSwapchain->swapchainFormat;
    depthFormat = inSwapchain->depthFormat;

    VkPipelineRenderingCreateInfo pipeline_rendering_create_info{};
    pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipeline_rendering_create_info.colorAttachmentCount = 1;
    pipeline_rendering_create_info.pColorAttachmentFormats = &colorFormat;
    pipeline_rendering_create_info.depthAttachmentFormat = depthFormat;

    // Assign it to both the main window and any secondary OS windows you drag out
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = pipeline_rendering_create_info;
    init_info.PipelineInfoForViewports.PipelineRenderingCreateInfo = pipeline_rendering_create_info;

    ImGui_ImplVulkan_LoadFunctions(
        VK_API_VERSION_1_3,
        [](const char* function_name, void* user_data) {
            return glfwGetInstanceProcAddress(*static_cast<VkInstance*>(user_data), function_name);
        },
        &inContext->instance
    );

    ImGui_ImplVulkan_Init(&init_info);

    UI::LoadFonts();

    return true;
}

UIRenderer::~UIRenderer()
{
    if (pContext && pContext->device)
    {
        vkDeviceWaitIdle(pContext->device);

        // CRITICAL: Force ImGui to destroy viewport command buffers before shutting down
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::DestroyPlatformWindows();
        }

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (imguiPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(pContext->device, imguiPool, nullptr);
            imguiPool = VK_NULL_HANDLE;
        }
    }
}

void UIRenderer::BeginUIFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIRenderer::EndUIFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void UIRenderer::RecordUICommands(VkCommandBuffer inCmdBuffer)
{
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), inCmdBuffer);
}

void UIRenderer::createDescriptorPool(VkDevice inDevice D_DEFN)
{
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(ImGuiPoolSizes); //1000?
    pool_info.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(ImGuiPoolSizes));
    pool_info.pPoolSizes = ImGuiPoolSizes;

    CHECK(vkCreateDescriptorPool(inDevice, &pool_info, nullptr, &imguiPool));

    SET_DNAME(inDevice, imguiPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL);
}
