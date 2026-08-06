// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "ModelLoader.h"

#include <stdexcept>
#include <iostream>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace ModelLoader
{
    // Only returns CPU data
    CPUMesh LoadGLTF(const std::string& filePath)
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

        CPUMesh mesh_data;
        if (data->meshes_count > 0)
        {
            const cgltf_mesh* mesh = &data->meshes[0];
            const cgltf_primitive* primitive = &mesh->primitives[0];

            glm::vec3 material_base_color = {1.0f, 1.0f, 1.0f}; // Default white
            if (primitive->material && primitive->material->has_pbr_metallic_roughness)
            {
                float* color_factor = primitive->material->pbr_metallic_roughness.base_color_factor;
                material_base_color = {color_factor[0], color_factor[1], color_factor[2]};
                std::cout << "Found material color: " << material_base_color.x << material_base_color.y << material_base_color.z << std::endl;
            }

            // Extract Vertices
            const cgltf_accessor* position_accessor = nullptr;
            [[maybe_unused]]const cgltf_accessor* color_accessor = nullptr;
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
                mesh_data.vertices.resize(position_accessor->count);
                for (cgltf_size i = 0; i < position_accessor->count; ++i)
                {
                    cgltf_accessor_read_float(position_accessor, i, &mesh_data.vertices[i].position.x, 3);

#if 0
                    if (color_accessor)
                    {
                        cgltf_accessor_read_float(color_accessor, i, &mesh_data.vertices[i].color.x, 3);
                    }
                    else // fallback
                    {
                        // meshData.vertices[i].color = glm::vec3(0.0f, 0.0f, 0.0f);
                        mesh_data.vertices[i].color = material_base_color;
                    }
#endif

                    // Read UV coordinates if they exists
                    if (uv_accessor)
                    {
                        cgltf_accessor_read_float(uv_accessor, i, &mesh_data.vertices[i].uv.x, 2);
                    }
                    else // fallback
                    {
                        mesh_data.vertices[i].uv = glm::vec2(0.0f, 0.0f);
                    }

                    if (normal_accessor)
                    {
                        cgltf_accessor_read_float(normal_accessor, i, &mesh_data.vertices[i].normal.x, 3);
                    }
                }
            }

            // Extract Indices
            if (primitive->indices)
            {
                const cgltf_accessor* index_accessor = primitive->indices;
                mesh_data.indices.resize(index_accessor->count);
                for (cgltf_size i = 0; i < index_accessor->count; ++i)
                {
                    mesh_data.indices[i] = static_cast<uint32_t>(cgltf_accessor_read_index(index_accessor, i));
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
            mesh_data.texturePath = base_dir + data->images[0].uri;
        }

        cgltf_free(data);
        return mesh_data;
    }
} //AnvilModelLoader
