// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilUIRenderer.h"

#include <algorithm>
#include <stdexcept>

#include <volk.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "AnvilSwapchain.h"
#include "AnvilVulkanContext.h"
#include "VulkanDebug.h"
#include "VulkanResult.h"

namespace
{
    namespace Color
    {
        /** Bright red for x axis. */
        inline constexpr ImU32 X_AXIS    = IM_COL32(255, 50, 50, 255);

        /** Bright green for y axis. */
        inline constexpr ImU32 Y_AXIS  = IM_COL32(50, 255, 50, 255);

        /** Light blue for z axis. Lighter to contrast with dark backgrounds */
        inline constexpr ImU32 Z_AXIS   = IM_COL32(50, 150, 255, 255);
    } //Color

    namespace Axis
    {
        inline constexpr glm::vec3 X = glm::vec3(1.0f, 0.0f, 0.0f);
        inline constexpr glm::vec3 Y = glm::vec3(0.0f, 1.0f, 0.0f);
        inline constexpr glm::vec3 Z = glm::vec3(0.0f, 0.0f, 1.0f);
    } //Axis

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

bool AnvilUIRenderer::initializeUIRenderer(AnvilVulkanContext* inContext, GLFWwindow* inWindow, AnvilSwapchain* inSwapchain)
{
    ptrAContext = inContext;

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
    init_info.QueueFamily = inContext->anvilGraphicsQueueIndex;
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
        &inContext->anvilInstance
    );

    ImGui_ImplVulkan_Init(&init_info);

    return true;
}

AnvilUIRenderer::~AnvilUIRenderer()
{
    if (ptrAContext->anvilDevice)
    {
        vkDeviceWaitIdle(ptrAContext->anvilDevice);
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
        vkDestroyDescriptorPool(ptrAContext->anvilDevice, imguiPool, nullptr);
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
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(ImGuiPoolSizes); //1000?
    pool_info.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(ImGuiPoolSizes));
    pool_info.pPoolSizes = ImGuiPoolSizes;

    CHECK(vkCreateDescriptorPool(inDevice, &pool_info, nullptr, &imguiPool));

    ANVIL_DEBUG_NAME(inDevice, imguiPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL);
}

void AnvilUIRenderer::DrawDebugAxis(const glm::mat4& viewMatrix)
{
    // TODO: Clean up the DrawDebugAxis function

    // Position a small transparent window in the bottom right
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float size = 100.0f;
    const ImVec2 window_position = ImVec2(
        viewport->WorkPos.x + viewport->WorkSize.x - size - 20.0f,
        viewport->WorkPos.y + viewport->WorkSize.y - size - 20.0f
    );

    ImGui::SetNextWindowPos(window_position, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(size, size));
    ImGui::SetNextWindowBgAlpha(0.0f); // Fully transparent
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); // No border

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("DebugAxis", nullptr, flags))
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Center of our 100x100 window
        const ImVec2 origin = ImVec2(window_position.x + size * 0.5f, window_position.y + size * 0.5f);
        const float line_length = 35.0f;

        // Transform World axes into View Space
        // By multiplying by the mat3 of the view matrix, we discard translation and keep only rotation
        const glm::mat3 view_rotation = glm::mat3(viewMatrix);
        const glm::vec3 x_axis = view_rotation * Axis::X;
        const glm::vec3 y_axis = view_rotation * Axis::Y;
        const glm::vec3 z_axis = view_rotation * Axis::Z;

        // Structure to help us sort by Z-depth
        struct AxisData { glm::vec3 dir; ImU32 color; const char* label; };
        AxisData axes[3] = {
            { x_axis, Color::X_AXIS,  "X" },
            { y_axis, Color::Y_AXIS,  "Y" },
            { z_axis, Color::Z_AXIS, "Z" }
        };

        // Sort by Z depth so the axis facing the camera draws ON TOP of the others
        // In standard OpenGL/GLM LookAt, -Z is forward. So bigger Z means closer to camera.
        std::sort(axes, axes + 3, [](const AxisData& a, const AxisData& b) {
            return a.dir.z > b.dir.z;
        });

        // 4. Draw the lines and text
        for(int i = 0; i < 3; ++i)
        {
            // ImGui +Y is down, but GLM view space +Y is up. So we subtract the Y component.
            ImVec2 endPos = ImVec2(
                origin.x + axes[i].dir.x * line_length,
                origin.y - axes[i].dir.y * line_length
            );

            // Draw line (thickness 2.0f)
            draw_list->AddLine(origin, endPos, axes[i].color, 2.0f);

            // Draw label slightly past the end of the line
            ImVec2 text_position = ImVec2(
                origin.x + axes[i].dir.x * (line_length + 10.0f) - 4.0f,
                origin.y - axes[i].dir.y * (line_length + 10.0f) - 6.0f
            );
            draw_list->AddText(text_position, axes[i].color, axes[i].label);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
