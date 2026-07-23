// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_INPUT_H
#define ANVIL_VK_INPUT_H

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class AnvilInput
{
private:
    static GLFWwindow* s_glfwWindow;

    // GLFW_KEY_LAST is 348, so 350 covers all keys
    static bool s_CurrentKeys[350];
    static bool s_PreviousKeys[350];

    static bool s_CurrentMouseButtons[8];
    static bool s_PreviousMouseButtons[8];

    static glm::vec2 s_MousePosition;
    static glm::vec2 s_MouseDelta;
    static bool s_FirstMouseUpdate;

public:
    static void InitializeInputSystem(GLFWwindow* inWindow);
    static void UpdateInputs();

    // KEYBOARD
    // Returns true if the key is held down
    static bool IsKeyPressed(int inKey);

    // Returns true if the key was pressed on the same frame
    static bool IsKeyPressed_Frame(int inKey);

    // TODO: May need functions that listen to key being let go instead of pressed

    // MOUSE
    // Returns true if the button is held pressed
    static bool IsMouseButtonPressed(int inButton);

    // Returns true if the button was pressed on the same frame
    static bool IsMouseButtonPressed_Frame(int inButton);

    static glm::vec2 GetMousePosition();
    static glm::vec2 GetMouseDelta();

    // Utility for the camera
    static void SetCursorMode(int inMode);
};

#endif //ANVIL_VK_INPUT_H
