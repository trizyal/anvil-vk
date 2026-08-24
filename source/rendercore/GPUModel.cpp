// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "GPUModel.h"

#include <iostream>

GPUModel::GPUModel(GPUModel&& other) noexcept
{
    *this = std::move(other);
}

GPUModel& GPUModel::operator=(GPUModel&& other) noexcept
{
    if (this != &other)
    {
        destroyGPUModel();

        pContext = other.pContext;
        textures = std::move(other.textures);
        gpuMeshes = std::move(other.gpuMeshes);
        gpuMaterials = std::move(other.gpuMaterials);
        drawItems = std::move(other.drawItems);
        jointBuffer = std::move(other.jointBuffer);

        // std::move because texture will be a class in future
        defaultWhiteTexture = std::move(other.defaultWhiteTexture);
        defaultNormalTexture = std::move(other.defaultNormalTexture);

        other.pContext = nullptr;
    }
    return *this;
}

void GPUModel::createGPUModel(VulkanContext& inContext, const CPUModel& inModel, const AnvilMaterial& inMaterial,
    const std::string& sceneBufferName, const GPUBuffer& sceneBuffer, const std::string& textureName)
{
    // Destroy the old vulkan objects
    destroyGPUModel();

    pContext = &inContext;

    createJointBuffer();
    createTextures(inModel);
    createMaterialDescriptorSets(inModel, inMaterial, sceneBufferName, sceneBuffer, textureName);
    createMeshesAndDrawItems(inModel);
}

void GPUModel::createGPUModel(VulkanContext& inContext, const CPUModel& inModel, const AnvilMaterial& inMaterial)
{
    // Destroy the old vulkan objects
    destroyGPUModel();

    pContext = &inContext;

    createJointBuffer();
    createTextures(inModel);
    createMaterialDescriptorSets(inModel, inMaterial);
    createMeshesAndDrawItems(inModel);
}

void GPUModel::destroyGPUModel()
{
    if (!pContext)
    {
        return;
    }

    jointBuffer.destroyBuffer();

    for (AnvilTexture& texture : textures)
    {
        texture.destroyAnvilTexture(pContext);
    }
    textures.clear();
    defaultWhiteTexture.destroyAnvilTexture(pContext);
    defaultNormalTexture.destroyAnvilTexture(pContext);

    for (GPUMesh& mesh : gpuMeshes)
    {
        mesh.destroyGPUMesh();
    }
    gpuMeshes.clear();

    gpuMaterials.clear();
    drawItems.clear();

    pContext = nullptr;
}

void GPUModel::updateTransforms(const CPUModel& inModel)
{
    // Fast path to sync animated matrices from CPU to GPU draw items
    for (GPUModelDrawItem& item : drawItems)
    {
        if (item.cpuNodeIndex >= 0 && item.cpuNodeIndex < static_cast<int>(inModel.nodes.size()))
        {
            item.worldMatrix = inModel.nodes[item.cpuNodeIndex].worldMatrix;
        }
    }
}

void GPUModel::updateJoints(const CPUModel& inModel) const
{
    if (jointBuffer.buffer != VK_NULL_HANDLE && !inModel.skins.empty())
    {
        std::vector<glm::mat4> jointMatrices;

        // Find the first node that is rigged to a skeleton
        for (int i = 0; i < static_cast<int>(inModel.nodes.size()); ++i)
        {
            if (inModel.nodes[i].skinIndex >= 0)
            {
                inModel.computeJointMatrices(i, jointMatrices);
                break;
            }
        }

        // Upload to the Vulkan buffer
        if (!jointMatrices.empty())
        {
            // Calculate size, clamping it to the 256 bones we allocated
            size_t copySize = jointMatrices.size() * sizeof(glm::mat4);
            constexpr size_t maxBufferSize = MAX_BONES * sizeof(glm::mat4);
            if (copySize > maxBufferSize)
            {
                copySize = maxBufferSize;
            }

            void* data = nullptr;
            vmaMapMemory(pContext->allocator, jointBuffer.allocation, &data);
            std::memcpy(data, jointMatrices.data(), copySize);
            vmaUnmapMemory(pContext->allocator, jointBuffer.allocation);
        }
    }
}

void GPUModel::createTextures(const CPUModel& inModel)
{
    defaultWhiteTexture = TextureLoader::CreateSolidColorTexture(WhiteColor, *pContext);
    defaultNormalTexture = TextureLoader::CreateSolidColorTexture(NormalColor, *pContext);

    textures.reserve(inModel.textures.size());
    for (const CPUTexture& cpu_texture : inModel.textures)
    {
        try
        {
            textures.push_back(TextureLoader::LoadTexture(cpu_texture.imagePath, *pContext));
        }
        catch (...)
        {
            textures.push_back(defaultWhiteTexture);
        }
    }
}

void GPUModel::createMaterialDescriptorSets(const CPUModel& inModel, const AnvilMaterial& inMaterial,
    const std::string& sceneBufferName, const GPUBuffer& sceneBuffer, const std::string& textureName)
{
    gpuMaterials.clear();
    gpuMaterials.reserve(inModel.materials.size());

    for (size_t material_index = 0; material_index < inModel.materials.size(); material_index++)
    {
        const CPUMaterial& cpu_material = inModel.materials[material_index];

        GPUModelMaterial gpu_material;
        gpu_material.materialIndex = static_cast<int>(material_index);
        gpu_material.baseColorFactor = cpu_material.baseColorFactor;
        gpu_material.instance = inMaterial.createInstance();

        if (inMaterial.hasBinding(sceneBufferName))
        {
            gpu_material.instance.bindUniformBuffer(sceneBufferName, sceneBuffer);
        }

        if (inMaterial.hasBinding("jointMatrices"))
        {
            gpu_material.instance.bindStorageBuffer("jointMatrices", jointBuffer);
        }

        if (inMaterial.hasBinding(textureName))
        {
            if (cpu_material.baseColorTextureIndex >= 0 &&
                cpu_material.baseColorTextureIndex < static_cast<int>(textures.size()) &&
                textures[cpu_material.baseColorTextureIndex].imageView != VK_NULL_HANDLE)
            {
                gpu_material.instance.bindTexture(textureName, textures[cpu_material.baseColorTextureIndex]);
            }
            else if (!textures.empty() && textures[0].imageView != VK_NULL_HANDLE)
            {
                gpu_material.instance.bindTexture(textureName, textures[0]);
            }
            else
            {
                std::cerr << "Unhandled Stuff" << std::endl;
            }
        }

        gpu_material.instance.updateDescriptorSets();
        gpuMaterials.push_back(std::move(gpu_material));
    }
}

void GPUModel::createMaterialDescriptorSets(const CPUModel& inModel, const AnvilMaterial& inMaterial)
{
    // Configure Set 1 - Model Data
    if (inMaterial.hasSet(1))
    {
        modelSet = inMaterial.allocateSet(1);
        if (inMaterial.hasBinding("jointMatrices"))
        {
            modelSet.bindStorageBuffer("jointMatrices", jointBuffer);
        }
        modelSet.updateDescriptorSets();
    }

    // Configure Set 2 - Material Data
    gpuMaterials.clear();
    gpuMaterials.reserve(inModel.materials.size());
    for (size_t material_index = 0; material_index < inModel.materials.size(); material_index++)
    {
        const CPUMaterial& cpu_material = inModel.materials[material_index];
        GPUModelMaterial gpu_material;
        gpu_material.materialIndex = static_cast<int>(material_index);
        gpu_material.baseColorFactor = cpu_material.baseColorFactor;
        gpu_material.instance = inMaterial.allocateSet(2);

        // Base Color
        if (inMaterial.hasBinding("baseColorTexture"))
        {
            if (cpu_material.baseColorTextureIndex >= 0)
            {
                gpu_material.instance.bindTexture("baseColorTexture", textures[cpu_material.baseColorTextureIndex]);
            }
            else
            {
                gpu_material.instance.bindTexture("baseColorTexture", defaultWhiteTexture);
            }
        }

        // Normal Map
        if (inMaterial.hasBinding("normalTexture"))
        {
            if (cpu_material.normalTextureIndex >= 0)
            {
                gpu_material.instance.bindTexture("normalTexture", textures[cpu_material.normalTextureIndex]);
            }
            else
            {
                gpu_material.instance.bindTexture("normalTexture", defaultNormalTexture);
            }
        }

        gpu_material.instance.updateDescriptorSets();
        gpuMaterials.push_back(std::move(gpu_material));
    }

}

void GPUModel::createMeshesAndDrawItems(const CPUModel& inCPUModel)
{
    gpuMeshes.clear();
    drawItems.clear();

    std::vector<std::vector<uint32_t>> primitive_to_gpu_mesh;
    primitive_to_gpu_mesh.resize(inCPUModel.meshes.size()); // cannot use reserve here.

    for (size_t cpu_mesh_index = 0; cpu_mesh_index < inCPUModel.meshes.size(); cpu_mesh_index++)
    {
        const CPUMesh& cpu_mesh = inCPUModel.meshes[cpu_mesh_index];
        if (cpu_mesh.primitives.empty())
        {
            std::cerr << "Empty cpu_mesh.primitives" << std::endl;
        }
        primitive_to_gpu_mesh[cpu_mesh_index].reserve(cpu_mesh.primitives.size());

        for (const CPUMeshPrimitive& primitive : cpu_mesh.primitives)
        {
            GPUMesh gpu_mesh;
            gpu_mesh.createGPUMesh(*pContext, primitive);

            const uint32_t gpu_mesh_index = static_cast<uint32_t>(gpuMeshes.size());
            gpuMeshes.push_back(std::move(gpu_mesh));
            primitive_to_gpu_mesh[cpu_mesh_index].push_back(gpu_mesh_index);
        }
    }

    if (!inCPUModel.nodes.empty())
    {
        for (int node_index = 0; node_index < static_cast<int>(inCPUModel.nodes.size()); node_index++)
        {
            const CPUNode& node = inCPUModel.nodes[node_index];
            if (node.meshIndex < 0 || node.meshIndex >= static_cast<int>(inCPUModel.meshes.size()))
            {
                continue;
            }

            const CPUMesh& cpu_mesh = inCPUModel.meshes[node.meshIndex];
            const std::vector<uint32_t>& primitive_gpu_indices = primitive_to_gpu_mesh[node.meshIndex];

            for (size_t primitive_index = 0; primitive_index < cpu_mesh.primitives.size(); primitive_index++)
            {
                const CPUMeshPrimitive& primitive = cpu_mesh.primitives[primitive_index];

                GPUModelDrawItem draw_item;
                draw_item.gpuMeshIndex = primitive_gpu_indices[primitive_index];
                draw_item.gpuMaterialIndex = primitive.materialIndex;
                draw_item.worldMatrix = node.worldMatrix;
                draw_item.cpuNodeIndex = node_index;

                drawItems.push_back(draw_item);
            }
        }
        return;
    }

    // Fallback if no nodes
    for (size_t cpu_mesh_index = 0; cpu_mesh_index < inCPUModel.meshes.size(); cpu_mesh_index++)
    {
        const CPUMesh& cpu_mesh = inCPUModel.meshes[cpu_mesh_index];
        const std::vector<uint32_t>& primitive_gpu_indices = primitive_to_gpu_mesh[cpu_mesh_index];

        for (size_t primitive_index = 0; primitive_index < cpu_mesh.primitives.size(); primitive_index++)
        {
            const CPUMeshPrimitive& primitive = cpu_mesh.primitives[primitive_index];

            GPUModelDrawItem draw_item;
            draw_item.gpuMeshIndex = primitive_gpu_indices[primitive_index];
            draw_item.gpuMaterialIndex = primitive.materialIndex;
            draw_item.worldMatrix = glm::mat4(1.0f);
            draw_item.cpuNodeIndex = -1; // Unmapped

            drawItems.push_back(draw_item);
        }
    }
}

void GPUModel::createJointBuffer()
{
    // We must provide initial data because GPUBuffer::createBuffer always calls std::memcpy.
    // Initializing with Identity Matrices means vertices won't stretch to infinity on frame 0.
    std::vector<glm::mat4> initial_matrices(MAX_BONES, glm::mat4(1.0f));

    if (jointBuffer.buffer == VK_NULL_HANDLE)
    {
        jointBuffer.createBuffer(
            pContext->allocator,
            pContext->device,
            initial_matrices.data(),
            sizeof(glm::mat4) * MAX_BONES,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        );
    }
}
