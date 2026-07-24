#include <iostream>

#include "AnvilApplication.h"

#include "BoxModel.h"

int main()
{
    AnvilApplication anvil;
    anvil.initializeAnvil({
        .width = 1280,
        .height = 720,
        .title = "Anvil glTF Box Example"
    });

    BoxModel project;
    project.initalizeProject(anvil.getAnvilContext(), anvil.getAnvilSwapchain());

    // Register hot-reload event
    anvil.addShaderReloadCallback([&]() {
        project.loadPipeline();
    });

    try
    {
        anvil.runAnvil([&](VkCommandBuffer cmd, AnvilSwapchain* swapchain)
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
