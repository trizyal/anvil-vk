// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "TextureCube.h"

#include <iostream>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

#include "GPUMesh.h"
#include "ModelLoader.h"
#include "ShaderCompiler.h"
#include "TextureLoader.h"
#include "UIRenderer.h"

void TextureCube::initializeProject(VulkanContext& inAnvilContext, VulkanSwapchain& inAnvilSwapchain)
{
    ptrAContext = &inAnvilContext;
    ptrASwapchain = &inAnvilSwapchain;

    const char* modelPath = PROJECT_DIR "/Cube/glTF/Cube.gltf";
    const CPUMesh_Single cubeMesh = ModelLoader::LoadSingleMeshGLTF(modelPath);

    meshBuffer.createGPUMesh(*ptrAContext, cubeMesh);

    if (!cubeMesh.texturePath.empty())
    {
        std::cout << "Loading texture: " << cubeMesh.texturePath << std::endl;

        myTexture = TextureLoader::LoadTexture(
            cubeMesh.texturePath,
            *ptrAContext
        );
    }

    // Initialize shader compiler
    if (!shaderCompiler.initializeShaderCompiler())
    {
        throw std::runtime_error("Failed to initialize shader compiler!");
    }

    shaderCompiler.addSearchPath(PROJECT_DIR);
    setupDescriptors();
    loadPipeline();
}

void TextureCube::cleanupProject()
{
    if (ptrAContext)
    {
        myTexture.destroyAnvilTexture(ptrAContext);

        if (descriptorPool) {
            vkDestroyDescriptorPool(ptrAContext->anvilDevice, descriptorPool, nullptr);
        }
        if (descriptorSetLayout) {
            vkDestroyDescriptorSetLayout(ptrAContext->anvilDevice, descriptorSetLayout, nullptr);
        }

        meshBuffer.destroyGPUMesh();
        vkDestroyPipelineLayout(ptrAContext->anvilDevice, pipelineLayout, nullptr);
        vkDestroyPipeline(ptrAContext->anvilDevice, pipeline.pipeline, nullptr);
        vertexShader.destroyShaderModule();
        fragmentShader.destroyShaderModule();
    }
}

void TextureCube::recordCommands(VkCommandBuffer inCmd, VulkanSwapchain &inAnvilSwapchain)
{
    vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    // Set Dynamic States required by your AnvilPipelineBuilder
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(inAnvilSwapchain.swapchainExtent.width);
    viewport.height = static_cast<float>(inAnvilSwapchain.swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(inCmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = inAnvilSwapchain.swapchainExtent;
    vkCmdSetScissor(inCmd, 0, 1, &scissor);

    // Calculate C++ Transforms
    static float time = 0.0f;
    static float dt = 0.016f; // Simple delta time

    camera.updateCamera(dt);

    float aspect = static_cast<float>(inAnvilSwapchain.swapchainExtent.width) / static_cast<float>(inAnvilSwapchain.swapchainExtent.height);

    glm::mat4 projection = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();

    UIRenderer::DrawDebugAxis(view);

    PushConstants constants;
    constants.renderMatrix = projection * view;
    vkCmdPushConstants(inCmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &constants);

    vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(inCmd, 0, 1, &meshBuffer.vertexBuffer.buffer, &offset);

    // This was VK_INDEX_TYPE_UINT16, but everything else uses 32
    // Caused a bug where half the triangles were not being rendered.
    vkCmdBindIndexBuffer(inCmd, meshBuffer.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // Draw
    vkCmdDrawIndexed(inCmd, meshBuffer.indexCount, 1, 0, 0, 0);
}

void TextureCube::loadPipeline()
{
    std::cout << "Creating TextureCube pipeline." << std::endl;

    shaderCompiler.resetSession();

    // NO wait idle here. Anvil handled it.
    if (pipeline.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(ptrAContext->anvilDevice, pipeline.pipeline, nullptr);
        vkDestroyPipelineLayout(ptrAContext->anvilDevice, pipelineLayout, nullptr);
    }
    vertexShader.destroyShaderModule();
    fragmentShader.destroyShaderModule();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstantRange;
    // Add Descriptor Set Layout
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &descriptorSetLayout;

    if (vkCreatePipelineLayout(ptrAContext->anvilDevice, &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    // Create shader compilation request
    AnvilShaders::ShaderCompileRequest vReq{"TextureCube", "vertexMain", AnvilShaders::ST_Vertex};
    AnvilShaders::ShaderCompileRequest fReq{"TextureCube", "fragmentMain", AnvilShaders::ST_Fragment};

    // Compile shaders
    auto vSpirv = shaderCompiler.compileToSPIRV(vReq);
    auto fSpirv = shaderCompiler.compileToSPIRV(fReq);

    // Create shader modules
    vertexShader.createShaderModule(*ptrAContext, vSpirv);
    fragmentShader.createShaderModule(*ptrAContext, fSpirv);

    auto something = GPUMesh::getAttributeDescriptions();

    // Vertex Descriptions
    std::vector<VkVertexInputBindingDescription> bindings = {GPUMesh::getBindingDescription()};
    std::vector<VkVertexInputAttributeDescription> attributes =
        {something[0], something[1], something[2]};

    // Create pipeline
    PipelineBuilder pipelineBuilder;
    pipeline = pipelineBuilder.setShaders(vertexShader.get(), fragmentShader.get())
        .setVertexInput(bindings, attributes)
        .setColorAttachmentFormat(ptrASwapchain->swapchainFormat)
        .setDepthAttachmentFormat(ptrASwapchain->depthFormat)
        .enableDepthTest(true, VK_COMPARE_OP_LESS)
        .setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
        .disableBlending()
        .buildPipeline(ptrAContext->anvilDevice, pipelineLayout);
}

void TextureCube::setupDescriptors()
{
    // Create the Layout
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = 0;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &layoutBinding;

    if (vkCreateDescriptorSetLayout(ptrAContext->anvilDevice, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }

    // Create the Pool (Memory allocator for descriptor sets)
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    if (vkCreateDescriptorPool(ptrAContext->anvilDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool!");
    }

    // Allocate the actual Descriptor Set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &descriptorSetLayout;

    if (vkAllocateDescriptorSets(ptrAContext->anvilDevice, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set!");
    }

    // 4. Write to the Descriptor Set (Plug the actual texture into the slot)
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // The layout we transitioned to earlier
    imageInfo.imageView = myTexture.imageView;
    imageInfo.sampler = myTexture.sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(ptrAContext->anvilDevice, 1, &descriptorWrite, 0, nullptr);
}
