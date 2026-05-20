#ifndef ANVIL_VK_WINDOW_H
#define ANVIL_VK_WINDOW_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

class AnvilWindow
{
private:
    std::string anvilTitle;
    GLFWwindow* glfwWindow;
    uint32_t width;
    uint32_t height;

public:
    AnvilWindow(uint32_t inWidth, uint32_t inHeight, const std::string& inTitle);
    ~AnvilWindow();

    bool bShouldClose() const;
    void pollEvents() const;

    std::string getWindowTitle() const;
    GLFWwindow* getGLFWWindow() const;
    VkSurfaceKHR createSurface(VkInstance instance) const;
};

#endif //ANVIL_VK_WINDOW_H
