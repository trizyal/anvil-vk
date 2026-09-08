// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include <iostream>

#include "Anvil.h"
#include "SponzaDeferred.h"

int main()
{
    try
    {
        Anvil anvil;
        anvil.initializeAnvil({
            .width = 1280,
            .height = 720,
            .title = "Anvil Sponza"
        });

        SponzaDeferred project;
        project.initializeProject(anvil.getContext(), anvil.getSwapchain());

        // Register hot-reload event
        anvil.addShaderReloadCallback([&](std::string* outErrorLog) {
            return project.loadPipelines(outErrorLog);
        });

        RenderHooks hooks;
        hooks.onPreSwapchain = [&](VkCommandBuffer cmd, Swapchain* swapchain)
        {
            project.recordGeometryPass(cmd, *swapchain);
        };
        hooks.onSwapchain = [&](VkCommandBuffer cmd, Swapchain* swapchain)
        {
            project.recordLightingPass(cmd, *swapchain);
        };

        anvil.runAnvil(hooks);

        project.cleanupProject();
        anvil.shutdownAnvil();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
