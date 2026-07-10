#include <iostream>

#define VOLK_IMPLEMENTATION
#include <volk.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "AnvilApplication.h"

constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

int main()
{
    try
    {
        AnvilApplication app;

        app.initializeAnvil({
            .width = 1280,
            .height = 720,
            .title = "Anvil Vulkan Template"
        });

        app.runAnvil();
        app.shutdownAnvil();

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
