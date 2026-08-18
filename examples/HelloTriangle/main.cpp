#include <iostream>

#include "Anvil.h"

#include "HelloTriangle.h"

int main()
{
    Anvil anvil;
    anvil.initializeAnvil({
        .width = 1280,
        .height = 720,
        .title = "Anvil Hello Triangle Example"
    });

    HelloTriangle project;
    project.initalizeProject(anvil.getContext(), anvil.getSwapchain());

    // Register hot-reload event
    anvil.addShaderReloadCallback([&]() {
        project.loadPipeline();
    });

    try
    {
        anvil.runAnvil([&](VkCommandBuffer cmd, Swapchain* swapchain)
        {
            project.recordCommands(cmd, *swapchain);
        });
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    project.cleanupProject();
    anvil.shutdownAnvil();

    return EXIT_SUCCESS;
}
