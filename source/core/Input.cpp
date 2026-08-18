// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "Input.h"

#include <cstring>

// Define static variables
GLFWwindow* Input::s_glfwWindow = nullptr;
bool Input::s_CurrentKeys[KEY_COUNT] = {false};
bool Input::s_PreviousKeys[KEY_COUNT] = {false};
bool Input::s_CurrentMouseButtons[BUTTON_COUNT] = {false};
bool Input::s_PreviousMouseButtons[BUTTON_COUNT] = {false};
glm::vec2 Input::s_MousePosition = {0.0f, 0.0f};
glm::vec2 Input::s_MouseDelta = {0.0f, 0.0f};
bool Input::s_FirstMouseUpdate = true;

void Input::InitializeInputSystem(GLFWwindow* inWindow)
{
    s_glfwWindow = inWindow;
}

void Input::UpdateInputs()
{
    // Save previous frame's states
    std::memcpy(s_PreviousKeys, s_CurrentKeys, sizeof(s_CurrentKeys));
    std::memcpy(s_PreviousMouseButtons, s_CurrentMouseButtons, sizeof(s_CurrentMouseButtons));

    // Poll keyboard state, space is the first listed key
    for (int i = GLFW_KEY_SPACE; i <= GLFW_KEY_LAST; i++)
    {
        s_CurrentKeys[i] = (glfwGetKey(s_glfwWindow, i) == GLFW_PRESS);
    }

    // Poll mouse state
    for (int i = GLFW_MOUSE_BUTTON_1; i <= GLFW_MOUSE_BUTTON_LAST; i++)
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

bool Input::IsKeyPressed(const int inKey)
{
    return s_CurrentKeys[inKey];
}

bool Input::IsKeyPressed_Frame(const int inKey)
{
    return s_CurrentKeys[inKey] && !s_PreviousKeys[inKey];
}

bool Input::IsMouseButtonPressed(const int inButton)
{
    return s_CurrentMouseButtons[inButton];
}

bool Input::IsMouseButtonPressed_Frame(const int inButton)
{
    return s_CurrentMouseButtons[inButton] && !s_PreviousMouseButtons[inButton];
}

glm::vec2 Input::GetMousePosition()
{
    return s_MousePosition;
}

glm::vec2 Input::GetMouseDelta()
{
    return s_MouseDelta;
}

void Input::SetCursorMode(const int inMode)
{
    glfwSetInputMode(s_glfwWindow, GLFW_CURSOR, inMode);
}
