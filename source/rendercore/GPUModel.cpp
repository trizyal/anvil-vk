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

    createTextures(inModel);
    createMaterialDescriptorSets(inModel, inMaterial, sceneBufferName, sceneBuffer, textureName);
    createMeshesAndDrawItems(inModel);
}

void GPUModel::destroyGPUModel()
{
    if (!pContext)
    {
        return;
    }

    for (AnvilTexture& texture : textures)
    {
        texture.destroyAnvilTexture(pContext);
    }
    textures.clear();

    for (GPUMesh& mesh : gpuMeshes)
    {
        mesh.destroyGPUMesh();
    }
    gpuMeshes.clear();

    gpuMaterials.clear();
    drawItems.clear();

    pContext = nullptr;
}

void GPUModel::createTextures(const CPUModel& inModel)
{
    textures.reserve(inModel.textures.size());

    for (const CPUTexture& cpu_texture : inModel.textures)
    {
        if (cpu_texture.imagePath.empty())
        {
            textures.emplace_back();
            continue;
        }

        std::cout << "Loading model texture: " << cpu_texture.name << std::endl;
        textures.push_back(TextureLoader::LoadTexture(cpu_texture.imagePath, *pContext));
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
        for (const CPUNode& node : inCPUModel.nodes)
        {
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

                drawItems.push_back(draw_item);
            }
        }
        return;
    }

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

            drawItems.push_back(draw_item);
        }
    }
}
