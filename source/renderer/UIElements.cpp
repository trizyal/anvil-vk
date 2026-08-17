// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "UIElements.h"

#include <algorithm>

#include "imgui.h"

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
}

namespace UI
{
    void RenderWorldAxes(const glm::mat4& viewMatrix)
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
}
