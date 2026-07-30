// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_INPUT_H
#define ANVIL_VK_INPUT_H

/**
 * @file AnvilInput.h
 * @brief Keyboard and mouse input logic.
 */

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

/**
 * @brief Orchestrates keyboard and mouse input processing.
 *
 * Stores all keys and mouse buttons that return GLFW_PRESS in the current frame
 * and the previous frame. Stores mouse position and calculates mouse delta for
 * movement speed.
 */
class AnvilInput
{
public:
    AnvilInput() = default;
    ~AnvilInput() = default;

    // Delete Copy Operations
    AnvilInput(const AnvilInput&) = delete;
    AnvilInput& operator=(const AnvilInput&) = delete;

    // Delete Move Operations
    AnvilInput(AnvilInput&&) = delete;
    AnvilInput& operator=(AnvilInput&&) = delete;

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
    /**
     * @brief Store a pointer to GLFWwindow to be able to poll events.
     * @param inWindow Pointer to the GLFWwindow.
     */
    static void InitializeInputSystem(GLFWwindow* inWindow);

    /**
     * @brief Calculates and stores state of each keyboard and mouse input.
     */
    static void UpdateInputs();

    /**
     * @brief Check whether keyboard key is pressed.
     * @param inKey Key code for a keyboard input.
     * @return Returns true if the key is held down, otherwise false.
     */
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
