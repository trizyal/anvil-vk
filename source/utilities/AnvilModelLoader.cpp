// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilModelLoader.h"

#include <stdexcept>
#include <iostream>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace AnvilModelLoader
{
    // Only returns CPU data
    AnvilMesh LoadGLTF(const std::string& filePath)
    {
        cgltf_options options{};
        cgltf_data* data = nullptr;

        if (cgltf_parse_file(&options, filePath.c_str(), &data) != cgltf_result_success)
        {
            throw std::runtime_error("Failed to parse glTF file: " + filePath);
        }

        if (cgltf_load_buffers(&options, data, filePath.c_str()) != cgltf_result_success)
        {
            cgltf_free(data);
            throw std::runtime_error("Failed to load GLTF file: " + filePath);
        }

        AnvilMesh meshData;
        if (data->meshes_count > 0)
        {
            const cgltf_mesh* mesh = &data->meshes[0];
            const cgltf_primitive* primitive = &mesh->primitives[0];

            glm::vec3 materialBaseColor = {1.0f, 1.0f, 1.0f}; // Default white
            if (primitive->material && primitive->material->has_pbr_metallic_roughness)
            {
                float* colorFactor = primitive->material->pbr_metallic_roughness.base_color_factor;
                materialBaseColor = {colorFactor[0], colorFactor[1], colorFactor[2]};
                std::cout << "Found material color: " << materialBaseColor.x << materialBaseColor.y << materialBaseColor.z << std::endl;
            }

            // Extract Vertices
            const cgltf_accessor* position_accessor = nullptr;
            const cgltf_accessor* color_accessor = nullptr;
            const cgltf_accessor* uv_accessor = nullptr;
            const cgltf_accessor* normal_accessor = nullptr;

            for (cgltf_size i = 0; i < primitive->attributes_count; ++i)
            {
                if (primitive->attributes[i].type == cgltf_attribute_type_position)
                {
                    position_accessor = primitive->attributes[i].data;
                }
                else if (primitive->attributes[i].type == cgltf_attribute_type_color)
                {
                    color_accessor = primitive->attributes[i].data;
                }
                else if (primitive->attributes[i].type == cgltf_attribute_type_texcoord)
                {
                    uv_accessor = primitive->attributes[i].data;
                }
                else if (primitive->attributes[i].type == cgltf_attribute_type_normal)
                {
                    normal_accessor = primitive->attributes[i].data;
                }
            }

            if (position_accessor)
            {
                meshData.vertices.resize(position_accessor->count);
                for (cgltf_size i = 0; i < position_accessor->count; ++i)
                {
                    cgltf_accessor_read_float(position_accessor, i, &meshData.vertices[i].position.x, 3);

#if 0
                    if (color_accessor)
                    {
                        cgltf_accessor_read_float(color_accessor, i, &meshData.vertices[i].color.x, 3);
                    }
                    else // fallback
                    {
                        // meshData.vertices[i].color = glm::vec3(0.0f, 0.0f, 0.0f);
                        meshData.vertices[i].color = materialBaseColor;
                    }
#endif

                    // Read UV coordinates if they exists
                    if (uv_accessor)
                    {
                        cgltf_accessor_read_float(uv_accessor, i, &meshData.vertices[i].uv.x, 2);
                    }
                    else // fallback
                    {
                        meshData.vertices[i].uv = glm::vec2(0.0f, 0.0f);
                    }

                    if (normal_accessor)
                    {
                        cgltf_accessor_read_float(normal_accessor, i, &meshData.vertices[i].normal.x, 3);
                    }
                }
            }

            // Extract Indices
            if (primitive->indices)
            {
                const cgltf_accessor* index_accessor = primitive->indices;
                meshData.indices.resize(index_accessor->count);
                for (cgltf_size i = 0; i < index_accessor->count; ++i)
                {
                    meshData.indices[i] = static_cast<uint32_t>(cgltf_accessor_read_index(index_accessor, i));
                }
            }
        }

        if (data->images_count > 0 && data->images[0].uri != nullptr)
        {
            // Find the folder the .gltf is in
            std::string base_dir = "";
            size_t slash_pos = filePath.find_last_of("/\\");
            if (slash_pos != std::string::npos) {
                base_dir = filePath.substr(0, slash_pos + 1);
            }

            // Combine folder path + image name (e.g., "PROJECT_DIR/Box/glTF/Cube_BaseColor.png")
            meshData.texturePath = base_dir + data->images[0].uri;
        }

        cgltf_free(data);
        return meshData;
    }
} //AnvilModelLoader
