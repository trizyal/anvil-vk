// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

/**
 * @file RenderGraphStub.h
 * @brief Application wrapper demonstrating Deferred PBR rendering using the Render Graph architecture.
 */

#include "Application.h"
#include "Camera.h"
#include "GBuffer.h"
#include "GPUModel.h"
#include "Material.h"
#include "PipelineBuilder.h"
#include "Scene.h"
#include "SceneManager.h"
#include "ShaderProgram.h"

class RenderGraphApp
{
public:
    RenderGraphApp() = default;
    ~RenderGraphApp() = default;

private:
    Application app;

    SceneManager sceneManager;
    Camera camera;
    Scene scene;

    GBuffer gBuffer;


    // G-Buffer Geometry Pass Pipeline & Material
    ShaderProgram gBufferProgram;
    Material gBufferMaterial;
    AnvilPipeline gBufferPipeline;

    // Deferred Lighting Pass Pipeline & Material
    ShaderProgram deferredLightingProgram;
    Material deferredLightingMaterial;
    AnvilPipeline deferredLightingPipeline;
    MaterialInstance lightingSet0;

public:
    /**
     * @brief Initializes application subsystems, GPU resources, pipelines, and loads scene assets.
     */
    void initialize();

    /**
     * @brief Starts the main application loop, passing the RenderGraph recording hook to Application.
     */
    void run();

    /**
     * @brief Safely shuts down GPU resources and application state.
     */
    void cleanup();

private:
    bool buildPipelines(std::string* outErrorMessage);
    bool buildGBufferPipeline(std::string* outErrorMessage);
    bool buildLightingPipeline(std::string* outErrorMessage);

    /**
     * @brief Builds and executes the per-frame RenderGraph passes.
     */
    void recordRenderGraph(VkCommandBuffer cmd);

    /**
     * @brief Geometry pass callback: Renders the GPU model into the G-Buffer attachments.
     */
    void drawGBufferGeometry(VkCommandBuffer cmd);

    /**
     * @brief Lighting pass callback: Resolves G-Buffer lighting or executes deferred debug views.
     */
    void drawDeferredLighting(VkCommandBuffer cmd);
};
