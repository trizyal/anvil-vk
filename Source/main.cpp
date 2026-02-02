#include <iostream>

#define VOLK_IMPLEMENTATION
#include <volk.h>

#include <GLFW/glfw3.h>

#include <VkBootstrap.h>

int main() {
    // --------------------------------------------------
    // Initialize GLFW
    // --------------------------------------------------
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window = glfwCreateWindow(
        1280,
        720,
        "anvil-vk",
        nullptr,
        nullptr
    );

    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    // --------------------------------------------------
    // Initialize Volk (GLOBAL function loader)
    // --------------------------------------------------
    if (volkInitialize() != VK_SUCCESS) {
        std::cerr << "Failed to initialize Volk\n";
        return -1;
    }

    // --------------------------------------------------
    // Create Vulkan instance (vk-bootstrap)
    // --------------------------------------------------
    vkb::InstanceBuilder builder;

    auto inst_ret = builder
        .set_app_name("anvil-vk")
        .set_engine_name("anvil")
        .request_validation_layers(true)
        .use_default_debug_messenger()
        .build();

    if (!inst_ret) {
        std::cerr << "Failed to create Vulkan instance\n";
        return -1;
    }

    vkb::Instance vkb_instance = inst_ret.value();
    VkInstance instance = vkb_instance.instance;

    // --------------------------------------------------
    // Load instance-level Vulkan functions
    // --------------------------------------------------
    volkLoadInstance(instance);

    // --------------------------------------------------
    // Create Vulkan surface via GLFW
    // --------------------------------------------------
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan surface\n";
        return -1;
    }

    // --------------------------------------------------
    // Main loop (empty)
    // --------------------------------------------------
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    // --------------------------------------------------
    // Cleanup
    // --------------------------------------------------
    vkDestroySurfaceKHR(instance, surface, nullptr);

    vkb::destroy_instance(vkb_instance);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
