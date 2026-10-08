// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "Application.h"

#include "Console.h"
#include "Input.h"
#include "Logger.h"
#include "RenderDoc.h"
#include "ScreenLogger.h"
#include "Trace.h"
#include "UIElements.h"

// TODO: Should move these to relevant files.
CVAR_BOOL("attachrenderdoc", "Start the executable with RenderDoc attached.", true);
CVAR_BOOL("attachtracy", "Start the executable with Tracy attached.", true);
CVAR_INT("r.shadowmapsize", "Resolution of shadow maps.", 1048);
CVAR_INT("a.logverbosity", "0: Fatal, 1: Error, 2: Warning, 3: Info, 4: Debug, 5: Trace", 5);
CVAR_INT("r.debugmode", "0: None, 1: Base Color, 2: Raw Normal Maps, 3: World Normal, 4: Metallic, 5: Roughness, 6: Depth", 0);
CVAR_BOOL("r.freezerendering", "Freezes the rendering state on the frame.", false);
CVAR_BOOL("r.frustumculling", "Enable frustum culling.", true);

void Application::initialize(const CreateInfo& info)
{
    SCOPE_CPU;

    // TODO: This should come from a cvar/engineconfig.
    const std::string log_file = SAVED_DIR "/logs/log.txt";
    Logger::InitializeLogger(log_file);

    LOG_INFO("Initializing Application.");
    const auto cpu_start = std::chrono::high_resolution_clock::now();
    ENSURE(!applicationInitialized, "Application is already initialized.");

    window = std::make_unique<Window>(info.width, info.height, info.title);

    // TODO: Unify whether we want to pass * or &.
    context.initializeVulkanContext(*window);
    swapchain.initializeSwapchain(context, window->getFramebufferExtent());

    renderContext.initializeRenderContext(&context, &swapchain);
    uiRenderer.initializeUIRenderer(&context, window->getGLFWWindow(), &swapchain);

    shaderCompiler.initializeShaderCompiler();
    debugPass.initializeDebugPass(context, shaderCompiler, swapchain.swapchainFormat, swapchain.depthFormat, &activeShaderErrorLog);

    // TODO: RenderDoc attach check.
    RenderDoc::InitializeRenderDoc();
    Input::InitializeInputSystem(window->getGLFWWindow());
    Console::InitializeConsole();

    addShaderReloadCallback([this](std::string* err) -> bool
    {
        return reloadDebugShaders(err);
    });

    applicationInitialized = true;
    const auto cpu_end = std::chrono::high_resolution_clock::now();
    const auto init_time = std::chrono::duration<float, std::milli>(cpu_end - cpu_start).count();
    LOG_INFO("Anvil initialization complete.");
    LOG_INFO("Initialization took: {}ms", init_time);
}

void Application::shutdown()
{
    SCOPE_CPU;
    LOG_INFO("Shutting Down application.");

    ENSURE(applicationInitialized, "Application is not initialized.");

    vkDeviceWaitIdle(context.device);

    debugPass.cleanupDebugPass();
    shaderCompiler.shutdownShaderCompiler();
    Logger::ShutdownLogger();
    window.reset();

    applicationInitialized = false;
}

void Application::run(const RenderHooks& renderHooks)
{
    FATAL(applicationInitialized, "Application is not initialized.");

    while (!window->bShouldClose())
    {
        SCOPE_CPU_NAME("FRAME");
        const auto frame_start = std::chrono::high_resolution_clock::now();

        Window::pollEvents();
        Input::UpdateInputs();

        // FIX: Check for minimized window state.
        if (window->isMinimised())
        {
            // Skip the rest of the loop entirely.
            // ImGui never starts, rendering never happens.
            continue;
        }

        UIRenderer::BeginUIFrame();

        // TODO: Remove the UI calls from ScreenLogger and move that to the UI files.
        ScreenLogger::DrawOverlay();
        UI::DrawConsoleWindow(&consoleState);

        // Toggle Developer Console with the tilde key (~)
        if (Input::IsKeyPressed_Frame(GLFW_KEY_GRAVE_ACCENT))
        {
            consoleState = (consoleState + 1) % 3;
        }

        // Check for Shader Reload
        if (Input::IsKeyPressed(GLFW_KEY_LEFT_CONTROL) && Input::IsKeyPressed_Frame(GLFW_KEY_PERIOD))
        {
            triggerShaderHotReload();
        }

        // Render Error Dialog if hot reload fails.
        if (bShaderErrorModalOpen)
        {
            UI::DrawShaderErrorModal(activeShaderErrorLog,
                [this]()
                {
                    // Try again
                    triggerShaderHotReload();
                },
                [this]()
                {
                    // Abort and use previous setup
                    bShaderErrorModalOpen = false;
                    activeShaderErrorLog.clear();
                }
            );
        }

        // Core Orchestration
        {
            SCOPE_CPU_NAME("Rendering");
            VkCommandBuffer cmd = renderContext.beginFrame(*window);
            FATAL(cmd != VK_NULL_HANDLE, "Command Buffer Invalid.");

            engineStats.resetFrameStats();

            // RenderGraph
            if (renderHooks.onRecordFrame)
            {
                renderHooks.onRecordFrame(cmd);
            }

            // UI
            {
                SCOPE_GPU(renderContext.tracyVkCtx, cmd, "UI RenderPass");
                engineStats.fps = 1000.f / engineStats.frameTime;
                UI::FrameStats(engineStats);
                uiRenderer.recordUICommands(cmd, renderContext.imageIndex);
            }

            renderContext.endFrame();
        }

        UIRenderer::EndUIFrame();

        auto frame_end = std::chrono::high_resolution_clock::now();
        engineStats.frameTime = std::chrono::duration<float, std::milli>(frame_end - frame_start).count();
        engineStats.cpuTime = engineStats.frameTime; // Rough approximation, fine for now
        engineStats.gpuTime = renderContext.gpuProfiler.getGPUTime(renderContext.frameIndex);
    }

    vkDeviceWaitIdle(context.device);
}

void Application::triggerShaderHotReload()
{
    SCOPE_CPU;

    if (shaderReloadQueue.empty())
    {
        LOG_INFO("Shader reload queue is empty.");
        return;
    }

    LOGUI("Shader reload triggered. Pausing GPU.");
    vkDeviceWaitIdle(context.device);

    bool b_success = true;
    std::string errors;

    for (auto& callback : shaderReloadQueue)
    {
        std::string err;
        if (!callback(&err))
        {
            b_success = false;
            errors += err + "\n";
        }
    }

    if (b_success)
    {
        bShaderErrorModalOpen = false;
        activeShaderErrorLog.clear();
        LOGUI("Shaders successfully reloaded.");
    }
    else
    {
        bShaderErrorModalOpen = true;
        activeShaderErrorLog = errors;
        LOG_ERROR("Shader hot-reload failed.");
        LOGUI("Shader hot-reload failed!", AnvilColor::Red);
    }
}

bool Application::reloadDebugShaders(std::string* outError)
{
    SCOPE_CPU;
    LOG_DEBUG("Reloading Debug Shaders.");

    // Force Slang to drop its module cache and read from disk again
    shaderCompiler.resetSession();

    // Create a temporary pass and attempt to initialize it
    DebugPass temp_pass;
    const bool b_success = temp_pass.initializeDebugPass(context, shaderCompiler, swapchain.swapchainFormat, swapchain.depthFormat, outError);

    if (b_success)
    {
        LOG_INFO("Debug Shaders Reload successfull.");

        // Safely tear down the old pipelines first.
        debugPass.cleanupDebugPass();

        // Transfer ownership of the new Vulkan handles to the active debugPass.
        debugPass = std::move(temp_pass);

        // Clear cached G-Buffer view so the new deferred descriptor set knows to rebind it.
        debugPass.cachedGBufferView = VK_NULL_HANDLE;
    }
    else
    {
        LOG_ERROR("Debug Shaders Reload failed.");

        // Clean up whatever partially compiled in the temporary pass.
        temp_pass.cleanupDebugPass();
    }

    return b_success;
}
