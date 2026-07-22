// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "AnvilMeshLoader.h"

#include <stdexcept>

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

            // Extract Vertices
            cgltf_accessor* positionAccessor = nullptr;
            for (cgltf_size i = 0; i < primitive->attributes_count; ++i)
            {
                if (primitive->attributes[i].type == cgltf_attribute_type_position)
                {
                    positionAccessor = primitive->attributes[i].data;
                    break;
                }
            }

            if (positionAccessor)
            {
                meshData.vertices.resize(positionAccessor->count);
                for (cgltf_size i = 0; i < positionAccessor->count; ++i)
                {
                    cgltf_accessor_read_float(positionAccessor, i, &meshData.vertices[i].position.x, 3);

                    // TODO: Read color in from glTF
                    meshData.vertices[i].color = {0.05f, 0.05f, 0.05f}; // Default white
                }
            }

            // Extract Indicies
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
