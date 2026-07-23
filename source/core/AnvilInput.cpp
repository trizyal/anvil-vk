// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilInput.h"

#include <cstring>

// Define static variables
GLFWwindow* AnvilInput::s_glfwWindow = nullptr;
bool AnvilInput::s_CurrentKeys[350] = {false};
bool AnvilInput::s_PreviousKeys[350] = {false};
bool AnvilInput::s_CurrentMouseButtons[8] = {false};
bool AnvilInput::s_PreviousMouseButtons[8] = {false};
glm::vec2 AnvilInput::s_MousePosition = {0.0f, 0.0f};
glm::vec2 AnvilInput::s_MouseDelta = {0.0f, 0.0f};
bool AnvilInput::s_FirstMouseUpdate = true;

void AnvilInput::InitializeInputSystem(GLFWwindow* inWindow)
{
    s_glfwWindow = inWindow;
}

void AnvilInput::UpdateInputs()
{
    // Save previous frame's states
    std::memcpy(s_PreviousKeys, s_CurrentKeys, sizeof(s_CurrentKeys));
    std::memcpy(s_PreviousMouseButtons, s_CurrentMouseButtons, sizeof(s_CurrentMouseButtons));

    // Poll keyboard state, space is the first listed key
    for (int i = GLFW_KEY_SPACE; i < GLFW_KEY_LAST; i++)
    {
        s_CurrentKeys[i] = (glfwGetKey(s_glfwWindow, i) == GLFW_PRESS);
    }

    // Poll mouse state
    for (int i = GLFW_MOUSE_BUTTON_1; i < GLFW_MOUSE_BUTTON_LAST; i++)
    {
        s_CurrentMouseButtons[i] = (glfwGetMouseButton(s_glfwWindow, i) == GLFW_PRESS);
    }

    // Update mouse position and delta
    double xpos, ypos;
    glfwGetCursorPos(s_glfwWindow, &xpos, &ypos);

    if (s_FirstMouseUpdate)
    {
        s_MousePosition = glm::vec2(xpos, ypos);
        s_FirstMouseUpdate = false;
    }

    glm::vec2 newMousePosition(xpos, ypos);

    // Y is inverted because screen coordinates go top-to-bottom
    s_MouseDelta = glm::vec2(newMousePosition.x - s_MousePosition.x, s_MousePosition.y - newMousePosition.y);
    s_MousePosition = newMousePosition;
}

bool AnvilInput::IsKeyPressed(const int inKey)
{
    return s_CurrentKeys[inKey];
}

bool AnvilInput::IsKeyPressed_Frame(const int inKey)
{
    return s_CurrentKeys[inKey] && !s_PreviousKeys[inKey];
}

bool AnvilInput::IsMouseButtonPressed(const int inButton)
{
    return s_CurrentMouseButtons[inButton];
}

bool AnvilInput::IsMouseButtonPressed_Frame(const int inButton)
{
    return s_CurrentMouseButtons[inButton] && !s_PreviousMouseButtons[inButton];
}

glm::vec2 AnvilInput::GetMousePosition()
{
    return s_MousePosition;
}

glm::vec2 AnvilInput::GetMouseDelta()
{
    return s_MouseDelta;
}

void AnvilInput::SetCursorMode(const int inMode)
{
    glfwSetInputMode(s_glfwWindow, GLFW_CURSOR, inMode);
}
