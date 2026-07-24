// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilMeshLoader.h"

#include <stdexcept>
#include <iostream>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace AnvilModelLoader
{
    // Only returns CPU data
    AnvilMesh LoadGLTF(const std::string& filePath)
    {
        cgltf_options options = {};
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
            cgltf_mesh* mesh = &data->meshes[0];
            cgltf_primitive* primitive = &mesh->primitives[0];

            glm::vec3 materialBaseColor = {1.0f, 1.0f, 1.0f}; // Default white
            if (primitive->material && primitive->material->has_pbr_metallic_roughness)
            {
                float* colorFactor = primitive->material->pbr_metallic_roughness.base_color_factor;
                materialBaseColor = {colorFactor[0], colorFactor[1], colorFactor[2]};
                std::cout << "Found material color: " << materialBaseColor.x << materialBaseColor.y << materialBaseColor.z << std::endl;
            }

            // Extract Vertices
            cgltf_accessor* positionAccessor = nullptr;
            cgltf_accessor* colorAccessor = nullptr;
            cgltf_accessor* uvAccessor = nullptr;

            for (cgltf_size i = 0; i < primitive->attributes_count; ++i)
            {
                if (primitive->attributes[i].type == cgltf_attribute_type_position)
                {
                    positionAccessor = primitive->attributes[i].data;
                }
                else if (primitive->attributes[i].type == cgltf_attribute_type_color)
                {
                    colorAccessor = primitive->attributes[i].data;
                }
                else if (primitive->attributes[i].type == cgltf_attribute_type_texcoord)
                {
                    uvAccessor = primitive->attributes[i].data;
                }
            }

            if (positionAccessor)
            {
                meshData.vertices.resize(positionAccessor->count);
                for (cgltf_size i = 0; i < positionAccessor->count; ++i)
                {
                    cgltf_accessor_read_float(positionAccessor, i, &meshData.vertices[i].position.x, 3);

                    if (colorAccessor)
                    {
                        cgltf_accessor_read_float(colorAccessor, i, &meshData.vertices[i].color.x, 3);
                    }
                    else // fallback
                    {
                        // meshData.vertices[i].color = glm::vec3(0.0f, 0.0f, 0.0f);
                        meshData.vertices[i].color = materialBaseColor;
                    }

                    // Read UV coordinates if they exists
                    if (uvAccessor)
                    {
                        cgltf_accessor_read_float(uvAccessor, i, &meshData.vertices[i].uv.x, 2);
                    }
                    else // fallback
                    {
                        meshData.vertices[i].uv = glm::vec2(0.0f, 0.0f);
                    }
                }
            }

            // Extract Indices
            if (primitive->indices)
            {
                cgltf_accessor* indexAccessor = primitive->indices;
                meshData.indices.resize(indexAccessor->count);
                for (cgltf_size i = 0; i < indexAccessor->count; ++i)
                {
                    meshData.indices[i] = static_cast<uint32_t>(cgltf_accessor_read_index(indexAccessor, i));
                }
            }
        }

        cgltf_free(data);
        return meshData;
    }
}
