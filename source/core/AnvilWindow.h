#ifndef ANVIL_VK_WINDOW_H
#define ANVIL_VK_WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

class AnvilWindow
{
private:
    GLFWwindow* glfwWindow;
    uint32_t width;
    uint32_t height;
    std::string anvilTitle;

public:
    AnvilWindow(uint32_t inWidth, uint32_t inHeight, std::string inTitle);
    ~AnvilWindow();

    [[nodiscard]] bool bShouldClose() const;
    static void pollEvents();

    [[nodiscard]] std::string getWindowTitle() const;
    [[nodiscard]] GLFWwindow* getGLFWWindow() const;
    VkSurfaceKHR createSurface(VkInstance instance) const;
};

#endif //ANVIL_VK_WINDOW_H
