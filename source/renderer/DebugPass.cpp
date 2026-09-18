// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "DebugPass.h"

#include <iostream>

#include "DebugModes.h"
#include "GPUMesh.h"
#include "GBuffer.h"
#include "GPUModel.h"
#include "VulkanContext.h"

bool DebugPass::initializeDebugPass(VulkanContext& inContext, ShaderCompiler& inCompiler, VkFormat swapchainFormat,
                                    VkFormat depthFormat, std::string* outError)
{
    pContext = &inContext;
    bool bSuccess = true;

    // Deferred Fullscreen Debug Pipeline
    AnvilShaders::ShaderCompileRequest def_v{"DebugDeferred", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest def_f{"DebugDeferred", "fragmentMain", AnvilShaders::ST_Fragment};

    if (program_Deferred.buildProgram(*pContext, inCompiler, def_v, def_f, outError))
    {
        material_Deferred.buildMaterialFromProgram(*pContext, program_Deferred);

        PipelineBuilder builder;
        pipeline_Deferred = builder.setShaders(material_Deferred.getVertexShader(),
                                               material_Deferred.getFragmentShader())
                .setVertexInput({}, {})
                .setColorAttachmentFormats({swapchainFormat})
                .setDepthAttachmentFormat(depthFormat)
                .enableDepthTest(false, VK_COMPARE_OP_ALWAYS)
                .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .setPolygonMode(VK_POLYGON_MODE_FILL)
                .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .disableBlending()
                .buildPipeline(pContext->device,
                              material_Deferred.materialPipelineLayout DNAME(
                                  "EngineDeferredDebug"));
    }
    else
    {
        bSuccess = false;
    }

    // Forward Geometry Debug Pipelines
    AnvilShaders::ShaderCompileRequest fwd_v{"DebugForward", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest fwd_f{"DebugForward", "fragmentMain", AnvilShaders::ST_Fragment};

    if (program_Forward.buildProgram(*pContext, inCompiler, fwd_v, fwd_f, outError))
    {
        material_Forward.buildMaterialFromProgram(*pContext, program_Forward);

        std::vector<VkVertexInputAttributeDescription> attributes = GPUMesh::GetAttributeDescriptions({POSITION, NORMAL, TANGENT, UV});
        std::vector<VkVertexInputBindingDescription> bindings = {GPUMesh::GetBindingDescription()};

        PipelineBuilder builder;
        builder.setShaders(material_Forward.getVertexShader(), material_Forward.getFragmentShader())
               .setVertexInput(bindings, attributes)
               .setColorAttachmentFormats({swapchainFormat})
               .setDepthAttachmentFormat(depthFormat)
               .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
               .setPolygonMode(VK_POLYGON_MODE_FILL)
               .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

        pipeline_Forward_Opaque = builder.enableDepthTest(true, VK_COMPARE_OP_LESS)
                .disableBlending()
                .buildPipeline(pContext->device, material_Forward.materialPipelineLayout DNAME("EngineForwardOpaqueDebug"));

        pipeline_Forward_Overdraw = builder.enableDepthTest(false, VK_COMPARE_OP_LESS)
                .enableAdditiveBlending()
                .buildPipeline(pContext->device, material_Forward.materialPipelineLayout DNAME("EngineForwardOverdrawDebug"));

        pipeline_Forward_Overshading = builder.enableDepthTest(true, VK_COMPARE_OP_LESS)
                .enableAdditiveBlending()
                .buildPipeline(pContext->device, material_Forward.materialPipelineLayout DNAME("EngineForwardOvershadingDebug"));

        pipeline_Forward_Wireframe = builder.enableDepthTest(true, VK_COMPARE_OP_LESS)
                .setPolygonMode(VK_POLYGON_MODE_LINE)
                .disableBlending()
                .buildPipeline(pContext->device, material_Forward.materialPipelineLayout DNAME("EngineForwardWireframeDebug"));
    }
    else
    {
        bSuccess = false;
    }

    return bSuccess;
}

void DebugPass::cleanupDebugPass()
{
    if (pContext)
    {
        vkDeviceWaitIdle(pContext->device);

        pipeline_Deferred.destroy(pContext);
        material_Deferred.destroyMaterial();
        program_Deferred.destroyProgram();

        pipeline_Forward_Opaque.destroy(pContext);
        pipeline_Forward_Overdraw.destroy(pContext);
        pipeline_Forward_Overshading.destroy(pContext);
        pipeline_Forward_Wireframe.destroy(pContext);
        material_Forward.destroyMaterial();
        program_Forward.destroyProgram();
    }
}

bool DebugPass::isDeferredMode(uint32_t mode)
{
    switch (static_cast<DebugMode>(mode))
    {
    case DebugMode::None:
    case DebugMode::Count:
        return false;

    case DebugMode::BaseColor:
    case DebugMode::WorldNormal:
    case DebugMode::Metallic:
    case DebugMode::Roughness:
    case DebugMode::Depth:
        return true;

    case DebugMode::GeometryNormal:
    case DebugMode::RawNormalMap:
    case DebugMode::Overdraw:
    case DebugMode::Overshading:
    case DebugMode::Wireframe:
        return false;
    }
    // NO default case!

    std::cerr << "Debug mode value " << mode << " does not map to any DebugModes" << std::endl;
    return false;
}

bool DebugPass::isForwardMode(uint32_t mode)
{
    switch (static_cast<DebugMode>(mode))
    {
    case DebugMode::None:
    case DebugMode::Count:
        return false;

    case DebugMode::GeometryNormal:
    case DebugMode::RawNormalMap:
    case DebugMode::Overdraw:
    case DebugMode::Overshading:
    case DebugMode::Wireframe:
        return true;

    case DebugMode::BaseColor:
    case DebugMode::WorldNormal:
    case DebugMode::Metallic:
    case DebugMode::Roughness:
    case DebugMode::Depth:
        return false;
    }
    // NO default case!

    std::cerr << "Debug mode value " << mode << " does not map to any DebugModes" << std::endl;
    return false;
}

AnvilPipeline DebugPass::getForwardPipeline(uint32_t mode) const
{
    switch (static_cast<DebugMode>(mode))
    {
    case DebugMode::BaseColor:
    case DebugMode::GeometryNormal:
    case DebugMode::RawNormalMap:
    case DebugMode::WorldNormal:
    case DebugMode::Metallic:
    case DebugMode::Roughness:
    case DebugMode::Depth:
        return pipeline_Forward_Opaque;

    case DebugMode::Overdraw:
        return pipeline_Forward_Overdraw;

    case DebugMode::Overshading:
        return pipeline_Forward_Overshading;

    case DebugMode::Wireframe:
        return pipeline_Forward_Wireframe;

        // Explicitly cover the rest to prevent compiler warnings
    case DebugMode::None:
    case DebugMode::Count:
        return AnvilPipeline{.pipeline = VK_NULL_HANDLE};
    }

    return AnvilPipeline{.pipeline = VK_NULL_HANDLE};
}

VkPipelineLayout DebugPass::getForwardLayout() const
{
    return material_Forward.materialPipelineLayout;
}

void DebugPass::drawDeferredResolve(VkCommandBuffer cmd, GBuffer& gBuffer, uint32_t debugMode, const glm::vec4& camPos)
{
    // CRITICAL FIX: Only allocate from the pool if we haven't done it yet!
    if (set_Deferred.descriptorSet == VK_NULL_HANDLE)
    {
        set_Deferred = material_Deferred.allocateSet(0);
    }

    // CRITICAL FIX: Only update the descriptor set if the G-Buffer textures have changed!
    // Since window resizing calls vkDeviceWaitIdle, the GPU is guaranteed to be safe here.
    if (cachedGBufferView != gBuffer.albedo.imageView)
    {
        set_Deferred.bindTexture("gAlbedo", gBuffer.albedo);
        set_Deferred.bindTexture("gNormal", gBuffer.normal);
        set_Deferred.bindTexture("gPBR", gBuffer.pbr);
        set_Deferred.bindTexture("gWorldPosition", gBuffer.worldPosition);
        set_Deferred.updateDescriptorSets();

        cachedGBufferView = gBuffer.albedo.imageView;
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_Deferred.pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material_Deferred.materialPipelineLayout, 0, 1, &set_Deferred.descriptorSet, 0, nullptr);

    DebugDeferredPushConstants pc = {camPos, debugMode };
    vkCmdPushConstants(cmd, material_Deferred.materialPipelineLayout, material_Deferred.pushConstantStages, 0, sizeof(DebugDeferredPushConstants), &pc);
    vkCmdDraw(cmd, 3, 1, 0, 0);
}
