// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_APPLICATION_H
#define ANVIL_VK_APPLICATION_H

#include <cstdint>
#include <memory>
#include <string>

#include "AnvilWindow.h"
#include "AnvilVulkanContext.h"
#include "AnvilSwapchain.h"
#include "AnvilRenderer.h"
#include "HelloTriangle.h"

struct AnvilApplicationCreateInfo
{
    uint32_t width = 1280;
    uint32_t height = 720;
    std::string title = "Anvil Vulkan Application";
};

class AnvilApplication
{
private:
    std::unique_ptr<AnvilWindow> anvilWindow;
    AnvilVulkanContext anvilContext;
    AnvilSwapchain anvilSwapchain;
    AnvilRenderer anvilRenderer;

    // Testing the User Renderer
    HelloTriangle test;

    bool anvilInitialized = false;

public:
    AnvilApplication() = default;
    ~AnvilApplication();

    AnvilApplication(const AnvilApplication&) = delete;
    AnvilApplication& operator=(const AnvilApplication&) = delete;

    void initializeAnvil(const AnvilApplicationCreateInfo& inCreateInfo = {});
    void runAnvil();
    void shutdownAnvil();

    [[nodiscard]] AnvilWindow& getAnvilWindow() const;
    [[nodiscard]] AnvilVulkanContext& getAnvilContext();
    [[nodiscard]] AnvilSwapchain& getAnvilSwapchain();
    [[nodiscard]] AnvilRenderer& getAnvilRenderer();
};

#endif //ANVIL_VK_APPLICATION_H
