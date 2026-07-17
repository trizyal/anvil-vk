// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#ifndef ANVIL_VK_APPLICATION_H
#define ANVIL_VK_APPLICATION_H

#include <cstdint>
#include <memory>
#include <string>
#include <functional>

#include "AnvilWindow.h"
#include "AnvilVulkanContext.h"
#include "AnvilSwapchain.h"
#include "AnvilRenderer.h"

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

    bool anvilInitialized = false;

public:
    AnvilApplication() = default;
    ~AnvilApplication();

    AnvilApplication(const AnvilApplication&) = delete;
    AnvilApplication& operator=(const AnvilApplication&) = delete;

    void initializeAnvil(const AnvilApplicationCreateInfo& inCreateInfo = {});
    void runAnvilRenderer(const std::function<void(VkCommandBuffer, AnvilSwapchain*)>& drawCallback);
    void shutdownAnvil();

    [[nodiscard]] AnvilWindow& getAnvilWindow() const;
    [[nodiscard]] AnvilVulkanContext& getAnvilContext();
    [[nodiscard]] AnvilSwapchain& getAnvilSwapchain();
    [[nodiscard]] AnvilRenderer& getAnvilRenderer();

    void addShaderReloadCallback(std::function<void()> shaderCallback);

private:
    bool wasReloadPressed = false;
    std::vector<std::function<void()>> shaderReloadQueue;
};

#endif //ANVIL_VK_APPLICATION_H
