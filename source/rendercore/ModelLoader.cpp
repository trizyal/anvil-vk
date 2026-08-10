// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "ModelLoader.h"

#include <stdexcept>
#include <iostream>
#include <filesystem>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace
{
    std::string GetBaseDirectory(const std::string& filePath);

    void LoadTextures(CPUModel& model, const cgltf_data* gltf_data, const std::string& base_directory);
    void LoadMaterials(CPUModel& model, const cgltf_data* gltf_data);
    void LoadMeshes(CPUModel& cpu_model, const cgltf_data* gltf_data);
    void LoadNodes(CPUModel& cpu_model, const cgltf_data* gltf_data);

    int GetTextureIndex(const cgltf_data* data, const cgltf_texture* texture);
    int GetMaterialIndex(const cgltf_data* data, const cgltf_material* material);
    int GetMeshIndex(const cgltf_data* data, const cgltf_mesh* mesh);
    int GetNodeIndex(const cgltf_data* data, const cgltf_node* node);

    void ReadNodeTRS(CPUNode& cpu_node, const cgltf_node* gltf_node);
    void ComputeWorldMatrices(CPUModel& model, int nodeIndex, const glm::mat4& parentMatrix);
}

namespace ModelLoader
{
    CPUModel LoadGLTF(const std::string& filePath)
    {
        cgltf_options options{};
        cgltf_data* gltf_data = nullptr;

        if (cgltf_parse_file(&options, filePath.c_str(), &gltf_data) != cgltf_result_success)
        {
            throw std::runtime_error("Failed to parse glTF file: " + filePath);
        }

        if (cgltf_load_buffers(&options, gltf_data, filePath.c_str()) != cgltf_result_success)
        {
            cgltf_free(gltf_data);
            throw std::runtime_error("Failed to load GLTF file: " + filePath);
        }

        CPUModel model;
        const std::string base_directory = GetBaseDirectory(filePath);

        // Get textures from gltf
        model.textures.reserve(gltf_data->textures_count);
        LoadTextures(model, gltf_data, base_directory);

        // Get materials from gltf
        model.materials.reserve(gltf_data->materials_count);
        LoadMaterials(model, gltf_data);

        // Get mesh data from gltf
        model.meshes.reserve(gltf_data->meshes_count);
        LoadMeshes(model, gltf_data);

        // Get nodes from gltf
        model.nodes.resize(gltf_data->nodes_count);
        LoadNodes(model, gltf_data);

        // Get scene data from gltf
        const cgltf_scene* scene = gltf_data->scene;
        if (!scene && gltf_data->scenes_count > 0)
        {
            scene = &gltf_data->scenes[0];
        }

        if (scene)
        {
            model.sceneRootNodes.reserve(scene->nodes_count);
            for (cgltf_size root_index = 0; root_index < scene->nodes_count; ++root_index)
            {
                const int node_index = GetNodeIndex(gltf_data, scene->nodes[root_index]);
                if (node_index >= 0)
                {
                    model.sceneRootNodes.push_back(node_index);
                }
            }
        }

        for (const int root_node : model.sceneRootNodes)
        {
            // Root nodes don't have parents, so their world matrices have to be set.
            ComputeWorldMatrices(model, root_node, glm::mat4(1.0f));
        }

        cgltf_free(gltf_data);
        return model;
    }

    // Only returns CPU data
    CPUMesh_Single LoadSingleMeshGLTF(const std::string& filePath)
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

        CPUMesh_Single mesh_data;
        if (data->meshes_count > 0)
        {
            const cgltf_mesh* mesh = &data->meshes[0];
            const cgltf_primitive* primitive = &mesh->primitives[0];

            glm::vec3 material_base_color = {1.0f, 1.0f, 1.0f}; // Default white
            if (primitive->material && primitive->material->has_pbr_metallic_roughness)
            {
                float* color_factor = primitive->material->pbr_metallic_roughness.base_color_factor;
                material_base_color = {color_factor[0], color_factor[1], color_factor[2]};
                std::cout << "Found material color: " << material_base_color.x << material_base_color.y <<
                    material_base_color.z << std::endl;
            }

            // Extract Vertices
            const cgltf_accessor* position_accessor = nullptr;
            [[maybe_unused]] const cgltf_accessor* color_accessor = nullptr;
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
            if (slash_pos != std::string::npos)
            {
                base_dir = filePath.substr(0, slash_pos + 1);
            }

            // Combine folder path + image name (e.g., "PROJECT_DIR/Box/glTF/Cube_BaseColor.png")
            mesh_data.texturePath = base_dir + data->images[0].uri;
        }

        cgltf_free(data);
        return mesh_data;
    }
} //AnvilModelLoader

namespace
{
    std::string GetBaseDirectory(const std::string& filePath)
    {
        const std::filesystem::path path(filePath);
        const std::filesystem::path parent = path.parent_path();

        if (parent.empty())
        {
            return "";
        }

        return parent.string() + std::string(1, std::filesystem::path::preferred_separator);
    }

    void LoadTextures(CPUModel& model, const cgltf_data* gltf_data, const std::string& base_directory)
    {
        for (cgltf_size texture_index = 0; texture_index < gltf_data->textures_count; ++texture_index)
        {
            CPUTexture cpu_texture;
            const cgltf_texture& gltf_texture = gltf_data->textures[texture_index];

            if (gltf_texture.name)
            {
                cpu_texture.name = gltf_texture.name;
            }

            if (gltf_texture.image && gltf_texture.image->uri)
            {
                cpu_texture.imagePath = base_directory + gltf_texture.image->uri;
            }

            model.textures.push_back(cpu_texture);
        }
    }

    void LoadMaterials(CPUModel& model, const cgltf_data* gltf_data)
    {
        for (cgltf_size material_index = 0; material_index < gltf_data->materials_count; ++material_index)
        {
            CPUMaterial cpu_material;
            const cgltf_material& gltf_material = gltf_data->materials[material_index];

            if (gltf_material.name)
            {
                cpu_material.name = gltf_material.name;
            }

            if (gltf_material.has_pbr_metallic_roughness)
            {
                const cgltf_pbr_metallic_roughness& pbr = gltf_material.pbr_metallic_roughness;

                cpu_material.baseColorFactor = glm::vec4(
                    static_cast<float>(pbr.base_color_factor[0]),
                    static_cast<float>(pbr.base_color_factor[1]),
                    static_cast<float>(pbr.base_color_factor[2]),
                    static_cast<float>(pbr.base_color_factor[3])
                );

                cpu_material.metallicFactor = static_cast<float>(pbr.metallic_factor);
                cpu_material.roughnessFactor = static_cast<float>(pbr.roughness_factor);
                cpu_material.baseColorTextureIndex = GetTextureIndex(gltf_data, pbr.base_color_texture.texture);
            }

            model.materials.push_back(cpu_material);
        }
    }

    void LoadMeshes(CPUModel& cpu_model, const cgltf_data* gltf_data)
    {
        for (cgltf_size mesh_index = 0; mesh_index < gltf_data->meshes_count; ++mesh_index)
        {
            CPUMesh cpu_mesh;
            const cgltf_mesh& gltf_mesh = gltf_data->meshes[mesh_index];

            if (gltf_mesh.name)
            {
                cpu_mesh.name = gltf_mesh.name;
            }

            cpu_mesh.primitives.reserve(gltf_mesh.primitives_count);
            for (cgltf_size primitive_index = 0; primitive_index < gltf_mesh.primitives_count; ++primitive_index)
            {
                CPUMeshPrimitive cpu_mesh_primitive;
                const cgltf_primitive& gltf_primitive = gltf_mesh.primitives[primitive_index];

                if (gltf_primitive.type != cgltf_primitive_type_triangles)
                {
                    std::cerr << "Skipping non-triangle glTF primitive in mesh: " << cpu_mesh.name << std::endl;
                    continue;
                }

                const cgltf_accessor* position_accessor = nullptr;
                const cgltf_accessor* normal_accessor = nullptr;
                const cgltf_accessor* uv_accessor = nullptr;

                for (cgltf_size attribute_index = 0; attribute_index < gltf_primitive.attributes_count; ++attribute_index)
                {
                    const cgltf_attribute& attribute = gltf_primitive.attributes[attribute_index];

                    if (attribute.type == cgltf_attribute_type_position)
                    {
                        position_accessor = attribute.data;
                    }
                    else if (attribute.type == cgltf_attribute_type_normal)
                    {
                        normal_accessor = attribute.data;
                    }
                    else if (attribute.type == cgltf_attribute_type_texcoord && attribute.index == 0)
                    {
                        uv_accessor = attribute.data;
                    }
                    else
                    {
                        std::cerr << "Found attribute not accounted for in mesh: " << cpu_mesh.name << std::endl;
                    }
                }

                if (!position_accessor)
                {
                    std::cerr << "Skipping glTF primitive without POSITION in mesh: " << cpu_mesh.name << std::endl;
                    continue;
                }

                cpu_mesh_primitive.materialIndex = GetMaterialIndex(gltf_data, gltf_primitive.material);

                cpu_mesh_primitive.vertices.resize(position_accessor->count);
                for (cgltf_size vertex_index = 0; vertex_index < position_accessor->count; ++vertex_index)
                {
                    MeshVertex vertex{};
                    cgltf_accessor_read_float(position_accessor, vertex_index, &vertex.position.x, 3);

                    if (normal_accessor)
                    {
                        cgltf_accessor_read_float(normal_accessor, vertex_index, &vertex.normal.x, 3);
                    }
                    else
                    {
                        vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                    }

                    if (uv_accessor)
                    {
                        cgltf_accessor_read_float(uv_accessor, vertex_index, &vertex.uv.x, 2);
                    }
                    else
                    {
                        vertex.uv = glm::vec2(0.0f, 0.0f);
                    }

                    cpu_mesh_primitive.vertices[vertex_index] = vertex;
                }

                if (gltf_primitive.indices)
                {
                    const cgltf_accessor* index_accessor = gltf_primitive.indices;

                    cpu_mesh_primitive.indices.resize(index_accessor->count);
                    for (cgltf_size index_index = 0; index_index < index_accessor->count; ++index_index)
                    {
                        cpu_mesh_primitive.indices[index_index] = static_cast<uint32_t>(cgltf_accessor_read_index(index_accessor, index_index));
                    }
                }
                else
                {
                    cpu_mesh_primitive.indices.resize(cpu_mesh_primitive.vertices.size());
                    for (uint32_t index_index = 0; index_index < cpu_mesh_primitive.vertices.size(); ++index_index)
                    {
                        cpu_mesh_primitive.indices[index_index] = index_index;
                    }
                }

                cpu_mesh.primitives.push_back(std::move(cpu_mesh_primitive));
            }
            cpu_model.meshes.push_back(std::move(cpu_mesh));
        }
    }

    void LoadNodes(CPUModel& cpu_model, const cgltf_data* gltf_data)
    {
        for (cgltf_size node_index = 0; node_index < gltf_data->nodes_count; ++node_index)
        {
            CPUNode cpu_node = cpu_model.nodes[node_index];
            const cgltf_node& gltf_node = gltf_data->nodes[node_index];

            if (gltf_node.name)
            {
                cpu_node.name = gltf_node.name;
            }

            cpu_node.meshIndex = GetMeshIndex(gltf_data, gltf_node.mesh);
            cpu_node.parentIndex = GetNodeIndex(gltf_data, gltf_node.parent);
            ReadNodeTRS(cpu_node, &gltf_node);

            cpu_node.children.reserve(gltf_node.children_count);
            for (cgltf_size child_index = 0; child_index < gltf_node.children_count; ++child_index)
            {
                cpu_node.children.push_back(GetNodeIndex(gltf_data, gltf_node.children[child_index]));
            }
        }
    }

    int GetTextureIndex(const cgltf_data* data, const cgltf_texture* texture)
    {
        if (!texture)
        {
            return -1;
        }

        for (cgltf_size i = 0; i < data->textures_count; ++i)
        {
            if (&data->textures[i] == texture)
            {
                return static_cast<int>(i);
            }
        }

        return -1;
    }

    int GetMaterialIndex(const cgltf_data* data, const cgltf_material* material)
    {
        if (!material)
        {
            return -1;
        }

        for (cgltf_size i = 0; i < data->materials_count; ++i)
        {
            if (&data->materials[i] == material)
            {
                return static_cast<int>(i);
            }
        }

        return -1;
    }

    int GetMeshIndex(const cgltf_data* data, const cgltf_mesh* mesh)
    {
        if (!mesh)
        {
            return -1;
        }

        for (cgltf_size i = 0; i < data->meshes_count; ++i)
        {
            if (&data->meshes[i] == mesh)
            {
                return static_cast<int>(i);
            }
        }

        return -1;
    }

    int GetNodeIndex(const cgltf_data* data, const cgltf_node* node)
    {
        if (!node)
        {
            return -1;
        }

        for (cgltf_size i = 0; i < data->nodes_count; ++i)
        {
            if (&data->nodes[i] == node)
            {
                return static_cast<int>(i);
            }
        }

        return -1;
    }

    void ReadNodeTRS(CPUNode& cpu_node, const cgltf_node* gltf_node)
    {
        glm::vec3 translation(0.0f);
        if (gltf_node->has_translation)
        {
            translation = glm::vec3(
                static_cast<float>(gltf_node->translation[0]),
                static_cast<float>(gltf_node->translation[1]),
                static_cast<float>(gltf_node->translation[2])
            );
        }

        glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
        if (gltf_node->has_rotation)
        {
            rotation = glm::quat(
                static_cast<float>(gltf_node->rotation[3]),
                static_cast<float>(gltf_node->rotation[0]),
                static_cast<float>(gltf_node->rotation[1]),
                static_cast<float>(gltf_node->rotation[2])
            );
        }

        glm::vec3 scale(1.0f);
        if (gltf_node->has_scale)
        {
            scale = glm::vec3(
                static_cast<float>(gltf_node->scale[0]),
                static_cast<float>(gltf_node->scale[1]),
                static_cast<float>(gltf_node->scale[2])
            );
        }

        if (gltf_node->has_matrix)
        {
            glm::mat4 matrix(1.0f);

            for (int column = 0; column < 4; ++column)
            {
                for (int row = 0; row < 4; ++row)
                {
                    matrix[column][row] = static_cast<float>(gltf_node->matrix[column * 4 + row]);
                }
            }

            cpu_node.localMatrix = matrix;
        }
        else
        {
            const glm::mat4 t = glm::translate(glm::mat4(1.0f), translation);
            const glm::mat4 r = glm::mat4_cast(rotation); //toMat4 call mat4_cast under the hood, but is experimental
            const glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);

            cpu_node.localMatrix = t * r * s;
        }

        cpu_node.translation = translation;
        cpu_node.rotation = rotation;
        cpu_node.scale = scale;
    }

    void ComputeWorldMatrices(CPUModel& model, const int nodeIndex, const glm::mat4& parentMatrix)
    {
        CPUNode& node = model.nodes[nodeIndex];
        node.worldMatrix = parentMatrix * node.localMatrix;

        for (const int child_index : node.children)
        {
            // Some recursion, hopefully does not causes crashes
            ComputeWorldMatrices(model, child_index, node.worldMatrix);
        }
    }
}
