#include <iostream>

#define VOLK_IMPLEMENTATION
#include <volk.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "AnvilWindow.h"
#include "AnvilVulkanContext.h"
// #include <GLFW/glfw3.h>

// #include <VkBootstrap.h>

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

int main()
{
    try
    {
        AnvilWindow window(1280, 720, "anvil-vk");
        AnvilVulkanContext anvilContext;

        std::cout << "Initializing Vulkan..." << std::endl;
        anvilContext.initialise(window);
        std::cout << "Vulkan Initialized successfully!" << std::endl;

        while (!window.bShouldClose())
        {
            AnvilWindow::pollEvents();
        }

        vkDeviceWaitIdle(anvilContext.anvilDevice);
        anvilContext.cleanup();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
