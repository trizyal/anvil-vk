// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "UIElements.h"

#include <algorithm>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#include <imgui.h>

#include "Camera.h"
#include "Color.h"
#include "Console.h"
#include "DebugModes.h"
#include "DebugPass.h"
#include "RenderDoc.h"
#include "SceneManager.h"
#include "ScreenLogger.h"
#include "Trace.h"

namespace
{
    namespace Axis
    {
        inline constexpr glm::vec3 X = glm::vec3(1.0f, 0.0f, 0.0f);
        inline constexpr glm::vec3 Y = glm::vec3(0.0f, 1.0f, 0.0f);
        inline constexpr glm::vec3 Z = glm::vec3(0.0f, 0.0f, 1.0f);
    } //Axis

    int ConsoleInputCallback(ImGuiInputTextCallbackData* data);
}

void UI::LoadFonts()
{
    SCOPE_CPU;

    ImGuiIO& io = ImGui::GetIO();

    base = io.Fonts->AddFontFromFileTTF(FontPath, 22.0f);
    debugUI = io.Fonts->AddFontFromFileTTF(FontPath, 18.0f);

    if (base == nullptr)
    {
        std::cerr << "Warning: Failed to load font at " << FontPath << ". Falling back to default." << std::endl;
    }
}

void UI::ApplyAnvilTheme()
{
    SCOPE_CPU;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding = 6.0f;
    colors[ImGuiCol_WindowBg].w = 1.0f;
}

void UI::FrameStats(const ::FrameStats& stats, bool* pOpen)
{
    SCOPE_CPU;

    const float PAD = 10.0f;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;
    ImVec2 windowPos(workPos.x + workSize.x - PAD, workPos.y + PAD);

    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.6f); // UE5 transparent black

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("AnvilStatUnitOverlay", pOpen, flags))
    {
        auto getMetricColor = [](float ms) -> ImVec4
        {
            if (ms < 16.67f) return ImVec4(0.4f, 1.0f, 0.4f, 1.0f); // Green
            if (ms < 33.33f) return ImVec4(1.0f, 0.8f, 0.2f, 1.0f); // Yellow
            return ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red
        };

        ImGui::PushFont(base);
        ImGui::TextDisabled("STAT UNIT");
        ImGui::Separator();
        ImGui::PopFont();

        ImGui::PushFont(debugUI);

        ImGui::Text("FPS:");
        ImGui::SameLine(80.0f);
        ImGui::TextColored(getMetricColor(1000.0f / (stats.fps + 0.001f)), "%.1f", static_cast<double>(stats.fps));

        ImGui::Text("Frame:");
        ImGui::SameLine(80.0f);
        ImGui::TextColored(getMetricColor(stats.frameTime), "%.2f ms", static_cast<double>(stats.frameTime));

        ImGui::Text("CPU:");
        ImGui::SameLine(80.0f);
        ImGui::TextColored(getMetricColor(stats.cpuTime), "%.2f ms", static_cast<double>(stats.cpuTime));

        ImGui::Text("GPU:");
        ImGui::SameLine(80.0f);
        ImGui::TextColored(getMetricColor(stats.gpuTime), "%.2f ms", static_cast<double>(stats.gpuTime));

        ImGui::Separator();

        ImGui::Text("Draws:");
        ImGui::SameLine(80.0f);
        ImGui::Text("%u", stats.drawCalls);

        ImGui::Text("Prims:");
        ImGui::SameLine(80.0f);
        ImGui::Text("%u", stats.primitiveCount);
        ImGui::PopFont();
    }
    ImGui::End();
}

void UI::RenderWorldAxes(const glm::mat4& viewMatrix)
{
    SCOPE_CPU;

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
        struct AxisData
        {
            glm::vec3 dir;
            Color color;
            const char* label;
        };
        AxisData axes[3] = {
            {x_axis, Colors::Red, "X"},
            {y_axis, Colors::Green, "Y"},
            {z_axis, Colors::Blue, "Z"}
        };

        // Sort by Z depth so the axis facing the camera draws ON TOP of the others
        // In standard OpenGL/GLM LookAt, -Z is forward. So bigger Z means closer to camera.
        std::sort(axes, axes + 3, [](const AxisData& a, const AxisData& b)
        {
            return a.dir.z > b.dir.z;
        });

        // 4. Draw the lines and text
        for (int i = 0; i < 3; ++i)
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

void UI::DrawShaderErrorModal(const std::string& errorLog, const std::function<void()>& onRetry,
                          const std::function<void()>& onAbort)
{
    SCOPE_CPU;

    ImGui::OpenPopup("Shader Compilation Error");

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(750.0f, 450.0f), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Shader Compilation Error", nullptr, ImGuiWindowFlags_NoMove))
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextUnformatted("Slang Shader Compilation Failed!");
        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::TextWrapped(
            "Fix the errors in your shader source file and click 'Retry' or press Ctrl + . to attempt re-compilation.");
        ImGui::Spacing();

        const float footer_height = 40.0f;
        ImGui::BeginChild("ErrorLogRegion", ImVec2(0.0f, -footer_height), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
        ImGui::TextUnformatted(errorLog.c_str());
        ImGui::PopStyleColor();
        ImGui::EndChild();

        ImGui::Separator();

        if (ImGui::Button("Retry Re-compilation", ImVec2(180.0f, 0.0f)))
        {
            if (onRetry) onRetry();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Abort (Keep Old Pipeline)", ImVec2(200.0f, 0.0f)))
        {
            if (onAbort) onAbort();
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::EndPopup();
}

// Static UI state stored securely in the CPP file
static char s_ConsoleInputBuffer[256] = "";
static int s_HistoryPosition = -1;

void UI::DrawConsoleWindow(const int* pState)
{
    SCOPE_CPU;

    if (*pState == 0)
    {
        return;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const bool is_mini = (*pState == 1);
    if (is_mini)
    {
        // Snap to bottom as a single line
        const float input_height = 35.0f;
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - input_height));
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, input_height));
        ImGui::SetNextWindowBgAlpha(0.6f);
    }
    else
    {
        // Half screen at the top
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, viewport->WorkSize.y * 0.5f));
        ImGui::SetNextWindowBgAlpha(0.8f);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;
    if (!ImGui::Begin("Developer Console", nullptr, window_flags))
    {
        ImGui::PopStyleVar(2);
        ImGui::End();
        return;
    }

    if (!is_mini)
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::BgGrayDark.u32());
        const float footer_height = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height), false, ImGuiWindowFlags_HorizontalScrollbar))
        {
            for (const std::string& item : Console::GetLogHistory())
            {
                if (!item.empty() && item[0] == ']')
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, Colors::BgGray.u32());
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, Colors::White.u32());
                }

                ImGui::TextUnformatted(item.c_str());
                ImGui::PopStyleColor();
            }

            if (Console::ShouldScroll() || ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            {
                ImGui::SetScrollHereY(1.0f);
                Console::ClearScroll();
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    bool reclaim_focus = false;
    ImGui::PushItemWidth(-1.0f);

    ImGui::Text(">");
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_FrameBg, Colors::BgGrayDark.u32());
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Colors::BgGrayDark.u32());
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Colors::BgGrayDark.u32());
    ImGui::PushStyleColor(ImGuiCol_NavHighlight, Colors::BgGrayDark.u32());
    ImGui::PushStyleColor(ImGuiCol_Border, Colors::BgGrayDark.u32());
    ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackCharFilter;
    if (ImGui::InputText("##ConsoleInput", s_ConsoleInputBuffer, std::size(s_ConsoleInputBuffer), input_flags, ConsoleInputCallback))
    {
        std::string s = s_ConsoleInputBuffer;

        if (!s.empty())
        {
            Console::Execute(s);
            s_HistoryPosition = -1; // Reset history index on submit
        }

        s_ConsoleInputBuffer[0] = '\0';
        reclaim_focus = true;
    }

    ImGui::PopStyleColor(5);
    ImGui::PopItemWidth();

    ImGui::SetItemDefaultFocus();
    if (reclaim_focus || ImGui::IsWindowAppearing())
    {
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::PopStyleVar(2);
    ImGui::End();
}

void UI::DrawCameraDebug(Camera& camera, bool* pOpen)
{
    if (pOpen && !*pOpen)
    {
        // Pointer exists and is false
        return;
    }

    if (ImGui::Begin("Camera Debug", pOpen))
    {
        ImGui::Text("Position: %.3f, %.3f, %.3f",
                    static_cast<double>(camera.position.x),
                    static_cast<double>(camera.position.y),
                    static_cast<double>(camera.position.z));

        ImGui::Text("Rotation: Pitch %.3f, Yaw %.3f",
                    static_cast<double>(camera.getPitch()),
                    static_cast<double>(camera.getYaw()));

        ImGui::Separator();

        // Allow live editing right in the UI
        glm::vec3 pos = camera.position;
        if (ImGui::DragFloat3("Set Position", &pos.x, 0.1f))
        {
            camera.position = pos;
        }

        float pitch = camera.getPitch();
        float yaw = camera.getYaw();
        if (ImGui::DragFloat("Set Pitch", &pitch, 0.5f, -89.0f, 89.0f) ||
            ImGui::DragFloat("Set Yaw", &yaw, 0.5f))
        {
            camera.setTransform(pos, pitch, yaw);
        }
    }
    ImGui::End();
}

bool UI::bShowCameraDebug = false;

void UI::DrawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                Console::Execute("quit");
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Renderer"))
        {
            // Engine-level toggles
            bool bFreeze = Console::GetCVarBool("r.freezerendering");
            if (ImGui::MenuItem("Freeze Rendering", nullptr, &bFreeze))
            {
                Console::Execute("freezerendering");
            }

            bool bCulling = Console::GetCVarBool("r.frustumculling");
            if (ImGui::MenuItem("Frustum Culling", nullptr, &bCulling))
            {
                Console::SetCVarBool("r.frustumculling", bCulling);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::MenuItem("Camera Debug", nullptr, &bShowCameraDebug);

            if (ImGui::MenuItem("Dump VMA Stats"))
            {
                Console::Execute("vmastats");
            }

            if (RenderDoc::IsInitialized())
            {
                if (ImGui::MenuItem("Capture Frame (RenderDoc)"))
                {
                    RenderDoc::TriggerCapture();
                }
            }
            else
            {
                // Greys out the item and prevents clicking
                ImGui::BeginDisabled();
                ImGui::MenuItem("RenderDoc not connected");
                ImGui::EndDisabled();
            }

            if (ImGui::MenuItem("Open Tracy Profiler"))
            {
                // Launch the external Tracy profiler UI asynchronously.
                #ifdef _WIN32
                std::string tracyPath = std::string(TOOLS_DIR) + "/tracy/windows/tracy-profiler.exe";

                ShellExecuteA(nullptr, "open", tracyPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                // std::system(("start " + tracyPath).c_str());
                #elif defined(__linux__)
                const std::string tracyPath =
                    std::string(TOOLS_DIR) + "/tracy/linux/tracy-profiler";

                std::system(("\"" + tracyPath + "\" >/dev/null 2>&1 &").c_str());
                // Unimplemented
                #endif
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

bool UI::DrawScenesMenu(SceneManager& sceneManager)
{
    bool b_changed = false;
    Index32 current_active_scene = sceneManager.activeSceneIndex;
    auto scenes = sceneManager.availableScenes;
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Scenes"))
        {
            for (size_t i = 0; i < scenes.size(); ++i)
            {
                bool is_selected = (current_active_scene == static_cast<int>(i));
                if (ImGui::MenuItem(scenes[i].sceneName.c_str(), nullptr, is_selected))
                {
                    sceneManager.activeSceneIndex = static_cast<int>(i);
                    b_changed = true;
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    return b_changed;
}

void UI::DrawViewsMenu()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Views"))
        {
            const int current_mode = Console::GetCVarInt("r.debugmode");
            for (int i = 0; i < static_cast<int>(DebugMode::Count); ++i)
            {
                const bool is_selected = (current_mode == i);
                if (ImGui::MenuItem(DebugPass::GetDebugModeName(static_cast<DebugMode>(i)), nullptr, is_selected))
                {
                    LOG_DEBUG("Updating debug mode to: {}", DebugPass::GetDebugModeName(static_cast<DebugMode>(i)));
                    Console::SetCVarInt("r.debugmode", i);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void UI::DrawLogOverlay(const std::vector<UILogMessage>& messages)
{
    if (messages.empty())
    {
        return;
    }

    // Get the position of the main application window
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    const ImVec2 overlay_position = ImVec2(main_viewport->WorkPos.x + 10.f, main_viewport->WorkPos.y + 10.f);

    constexpr ImGuiWindowFlags logger_flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBackground;

    ImGui::SetNextWindowPos(overlay_position, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("UILogOverlay", nullptr, logger_flags))
    {
        // We re-lock ONLY while reading the text to prevent data races if a thread pushed mid-draw.
        // Because culling/erasing is already done, this iteration is lightning fast.

        for (const UILogMessage& message : messages)
        {
            // Smooth fade out in the last second
            ImVec4 draw_color = message.color;
            if (message.timeRemaining < 1.0f)
            {
                draw_color.w *= message.timeRemaining;
            }

            // Draw the text
            // Using ImGui::TextUnformatted is faster than ImGui::TextColored("%s")
            // because it avoids C-style vararg parsing.
            ImGui::PushStyleColor(ImGuiCol_Text, draw_color);
            ImGui::TextUnformatted(message.text.data(), message.text.data() + message.text.size());
            ImGui::PopStyleColor();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

namespace
{
    int ConsoleInputCallback(ImGuiInputTextCallbackData* data)
    {
        SCOPE_CPU;

        // Block the grave accent (`) and tilde (~) from being typed
        if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter)
        {
            if (data->EventChar == '`' || data->EventChar == '~')
            {
                return 1; // Return 1 to discard the character
            }
        }
        else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
        {
            const std::vector<std::string>& history = Console::GetCommandHistory();
            if (history.empty())
            {
                return 0;
            }

            const int previousHistoryPosition = s_HistoryPosition;
            if (data->EventKey == ImGuiKey_UpArrow)
            {
                if (s_HistoryPosition == -1)
                {
                    s_HistoryPosition = static_cast<int>(history.size()) - 1;
                }
                else if (s_HistoryPosition > 0)
                {
                    s_HistoryPosition--;
                }
            }
            else if (data->EventKey == ImGuiKey_DownArrow)
            {
                if (s_HistoryPosition != -1)
                {
                    if (++s_HistoryPosition >= static_cast<int>(history.size()))
                    {
                        s_HistoryPosition = -1;
                    }
                }
            }

            // Only update text buffer if the position changed
            if (previousHistoryPosition != s_HistoryPosition)
            {
                const std::string history_string = (s_HistoryPosition >= 0) ? history[static_cast<size_t>(s_HistoryPosition)] : "";
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, history_string.c_str());
            }
        }
        return 0;
    }
}
