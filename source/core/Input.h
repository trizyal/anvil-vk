// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_INPUT_H
#define ANVIL_VK_INPUT_H

/**
 * @file Input.h
 * @brief Keyboard and mouse input logic.
 */

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

/**
 * @brief Number of keyboard inputs.
 *
 * GLFW_KEY_SPACE = 32 is the first input listed for the keyboard.
 * GLFW_KEY_LAST = 348, could save the 33 bool memories.
 */
constexpr uint32_t KEY_COUNT = GLFW_KEY_LAST + 1;

/**
 * @brief Number of mouse button inputs.
 *
 * GLFW_MOUSE_BUTTON_1 = 0 is the first input listed for the mouse.
 * GLFW_MOUSE_BUTTON_LAST = 7.
 */
constexpr uint32_t BUTTON_COUNT = GLFW_KEY_LAST + 1;

/**
 * @brief Orchestrates keyboard and mouse input processing.
 *
 * Stores all keys and mouse buttons that return GLFW_PRESS in the current frame
 * and the previous frame. Stores mouse position and calculates mouse delta for
 * movement speed.
 *
 * @note This class is non-copyable and non-movable as all data is static.
 *
 * @todo May need functions that listen to key being released instead of pressed.
 */
class Input
{
public:
    Input() = default;
    ~Input() = default;

    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    Input(Input&&) = delete;
    Input& operator=(Input&&) = delete;

private:
    static GLFWwindow* s_glfwWindow;

    static bool s_CurrentKeys[KEY_COUNT];
    static bool s_PreviousKeys[KEY_COUNT];

    static bool s_CurrentMouseButtons[BUTTON_COUNT];
    static bool s_PreviousMouseButtons[BUTTON_COUNT];

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
     * @brief Check whether keyboard key is pressed or held.
     * @param inKey Key code for a keyboard input.
     * @return `true` if the key is held down, otherwise `false`.
     */
    static bool IsKeyPressed(int inKey);

    /**
     * @brief Check whether keyboard key was pressed in this exact frame.
     * @param inKey Key code for a keyboard input.
     * @return `true` if the key was pressed in this frame, otherwise `false`.
     */
    static bool IsKeyPressed_Frame(int inKey);

    /**
     * @brief Check whether mouse button is pressed or held.
     * @param inButton Button code for mouse input.
     * @return `true` if the button is held down, otherwise `false`.
     */
    static bool IsMouseButtonPressed(int inButton);

    /**
     * @brief Check whether mouse button was pressed in this exact frame.
     * @param inButton Button code for mouse input.
     * @return `true` if the button was pressed in this frame, otherwise `false`.
     */
    static bool IsMouseButtonPressed_Frame(int inButton);

    /**
     * @brief Gets the stored glfwGetCursorPos() value.
     * @return Mouse position in XY pixels.
     */
    static glm::vec2 GetMousePosition();

    /**
     * @brief Gets delta calculated with inverted-Y because GLFW screen coordinates go top-down.
     * @return Delta between mouse position from last frame and this.
     */
    static glm::vec2 GetMouseDelta();

    /**
     * @brief Wrapper for glfwSetInputMode() to set GLFW_CURSOR mode.
     * @param inMode One of GLFW_CURSOR_NORMAL, GLFW_CURSOR_HIDDEN,
     * GLFW_CURSOR_DISABLED or GLFW_CURSOR_CAPTURED.
     */
    static void SetCursorMode(int inMode);
};

#endif //ANVIL_VK_INPUT_H
