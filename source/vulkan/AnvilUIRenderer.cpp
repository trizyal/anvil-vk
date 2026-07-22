// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilUIRenderer.h"

#include <stdexcept>

#include <volk.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "AnvilSwapchain.h"
#include "AnvilVulkanContext.h"
#include "AnvilVulkanDebug.h"

VkDescriptorPoolSize imguiPoolSizes[] =
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

bool AnvilUIRenderer::initializeUIRenderer(AnvilVulkanContext* inContext, GLFWwindow* inWindow, AnvilSwapchain* inSwapchain)
{
    VkDevice device = inContext->anvilDevice;

    createDescriptorPool(device, "ImGuiDescriptorPool");

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
    init_info.ApiVersion = VK_API_VERSION_1_3;
    init_info.Instance = inContext->anvilInstance;
    init_info.PhysicalDevice = inContext->anvilPhysicalDevice;
    init_info.Device = device;
    init_info.QueueFamily = inContext->anvilGraphicsQueueFamily;
    init_info.Queue = inContext->anvilGraphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 3;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    // --- DYNAMIC RENDERING HOOKS ---
    init_info.UseDynamicRendering = true;

    // CRITICAL: We must store the format in a static variable (or a class member)
    // because ImGui's Viewport system will read this pointer LATER when you drag a window!
    colorFormat = inSwapchain->anvilImageFormat;
    depthFormat = inSwapchain->depthFormat;

    VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
    pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
    pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;

    // Assign it to both the main window and any secondary OS windows you drag out
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo;
    init_info.PipelineInfoForViewports.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo;

    ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_3, [](const char* function_name, void* user_data) {
        return glfwGetInstanceProcAddress(*static_cast<VkInstance*>(user_data), function_name);
    }, &inContext->anvilInstance);

    ImGui_ImplVulkan_Init(&init_info);

    return true;
}

void AnvilUIRenderer::shutdownUIRenderer(AnvilVulkanContext* inContext)
{
    if (inContext->anvilDevice)
    {
        vkDeviceWaitIdle(inContext->anvilDevice);
    }

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
        vkDestroyDescriptorPool(inContext->anvilDevice, imguiPool, nullptr);
        imguiPool = VK_NULL_HANDLE;
    }
}

void AnvilUIRenderer::BeginUIFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void AnvilUIRenderer::EndUIFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

void AnvilUIRenderer::RecordUICommands(VkCommandBuffer inCmdBuffer)
{
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), inCmdBuffer);
}

void AnvilUIRenderer::createDescriptorPool(VkDevice inDevice ANVIL_DEBUG_DEFN)
{
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000 * IM_ARRAYSIZE(imguiPoolSizes);
    poolInfo.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(imguiPoolSizes));
    poolInfo.pPoolSizes = imguiPoolSizes;

    if (vkCreateDescriptorPool(inDevice, &poolInfo, nullptr, &imguiPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create ImGui Descriptor Pool");
    }

    ANVIL_DEBUG_NAME(inDevice, imguiPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL);
}
