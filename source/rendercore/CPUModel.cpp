// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "CPUModel.h"

#include <stdexcept>
#include <iostream>
#include <filesystem>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace
{
    std::string GetBaseDirectory(const std::string& file_path);
    std::string JoinPath(const std::string& base_directory, const char* uri);

    void LoadTextures(CPUModel& model, const cgltf_data* gltf_data, const std::string& base_directory);
    void LoadMaterials(CPUModel& model, const cgltf_data* gltf_data);
    void LoadMeshes(CPUModel& cpu_model, const cgltf_data* gltf_data);
    void LoadNodes(CPUModel& cpu_model, const cgltf_data* gltf_data);
    void LoadSkins(CPUModel& cpu_model, const cgltf_data* gltf_data);
    void LoadAnimations(CPUModel& cpu_model, const cgltf_data* gltf_data);

    int GetTextureIndex(const cgltf_data* data, const cgltf_texture* texture);
    int GetMaterialIndex(const cgltf_data* data, const cgltf_material* material);
    int GetMeshIndex(const cgltf_data* data, const cgltf_mesh* mesh);
    int GetNodeIndex(const cgltf_data* data, const cgltf_node* node);
    int GetSkinIndex(const cgltf_data* data, const cgltf_skin* skin);

    glm::mat4 ConvertMatrix(const cgltf_float* cgltf_matrix);
    glm::mat4 MakeLocalMatrix(const CPUNode& cpu_node);
    void ReadNodeTRS(CPUNode& cpu_node, const cgltf_node* gltf_node);
    void ComputeWorldMatrices(CPUModel& cpu_model, int node_index, const glm::mat4& parent_matrix);
}

void CPUModel::loadGLTF(const std::string& filePath)
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

    const std::string base_directory = GetBaseDirectory(filePath);

    // Get textures from gltf
    textures.resize(gltf_data->textures_count);
    LoadTextures(*this, gltf_data, base_directory);

    // Get materials from gltf
    materials.resize(gltf_data->materials_count);
    LoadMaterials(*this, gltf_data);

    // Get mesh data from gltf
    meshes.resize(gltf_data->meshes_count);
    LoadMeshes(*this, gltf_data);

    // Get nodes from gltf
    nodes.resize(gltf_data->nodes_count);
    LoadNodes(*this, gltf_data);

    // Get skins from gltf
    skins.resize(gltf_data->skins_count);
    LoadSkins(*this, gltf_data);

    // Get scene data from gltf
    const cgltf_scene* scene = gltf_data->scene;
    if (!scene && gltf_data->scenes_count > 0)
    {
        scene = &gltf_data->scenes[0];
    }

    if (scene)
    {
        sceneRootNodes.reserve(scene->nodes_count);
        for (cgltf_size root_index = 0; root_index < scene->nodes_count; ++root_index)
        {
            const int node_index = GetNodeIndex(gltf_data, scene->nodes[root_index]);
            if (node_index >= 0)
            {
                sceneRootNodes.push_back(node_index);
            }
        }
    }

    if (sceneRootNodes.empty())
    {
        for (int nodeIndex = 0; nodeIndex < static_cast<int>(nodes.size()); ++nodeIndex)
        {
            if (nodes[nodeIndex].parentIndex < 0)
            {
                sceneRootNodes.push_back(nodeIndex);
            }
        }
    }

    // Get animations from gltf
    animations.resize(gltf_data->animations_count);
    LoadAnimations(*this, gltf_data);

    updateAllMatrices();

    cgltf_free(gltf_data);
}

void CPUModel::updateAllMatrices()
{
    for (const int rootNodeIndex : sceneRootNodes)
    {
        ComputeWorldMatrices(*this, rootNodeIndex, glm::mat4(1.0f));
    }
}

void CPUModel::applyAnimation(const int animationIndex, const float time)
{
    if (animationIndex < 0 || animationIndex >= static_cast<int>(animations.size()))
    {
        return;
    }

    const CPUAnimation& animation = animations[animationIndex];

    for (const CPUAnimationChannel& channel : animation.channels)
    {
        if (channel.targetNodeIndex < 0 || channel.keyframeTimes.size() < 2)
        {
            continue;
        }

        size_t previous_index = 0;
        size_t next_index = 1;
        float factor = 0.0f;

        // --- NEW CLAMPING LOGIC ---
        if (time <= channel.keyframeTimes.front())
        {
            // Before the first frame: hold the first frame
            previous_index = 0;
            next_index = 0;
            factor = 0.0f;
        }
        else if (time >= channel.keyframeTimes.back())
        {
            // After the last frame: hold the last frame
            previous_index = channel.keyframeTimes.size() - 1;
            next_index = channel.keyframeTimes.size() - 1;
            factor = 0.0f;
        }
        else
        {
            // Find the keyframes we are currently between
            for (size_t i = 0; i < channel.keyframeTimes.size() - 1; ++i)
            {
                if (time >= channel.keyframeTimes[i] && time <= channel.keyframeTimes[i + 1])
                {
                    previous_index = i;
                    next_index = i + 1;

                    const float t0 = channel.keyframeTimes[previous_index];
                    const float t1 = channel.keyframeTimes[next_index];
                    factor = (t1 > t0) ? (time - t0) / (t1 - t0) : 0.0f;
                    break;
                }
            }
        }

        // Apply to CPU node
        CPUNode& node = nodes[channel.targetNodeIndex];

        // Apply interpolation based on path
        if (channel.path == AnimationPath::Rotation)
        {
            glm::quat q0 = channel.keyframeRotations[previous_index];
            glm::quat q1 = channel.keyframeRotations[next_index];
            node.rotation = glm::normalize(glm::slerp(q0, q1, factor));
        }
        else if (channel.path == AnimationPath::Translation)
        {
            glm::vec3 v0 = channel.keyframeTranslations[previous_index];
            glm::vec3 v1 = channel.keyframeTranslations[next_index];
            node.translation = glm::mix(v0, v1, factor); // Linear interpolation
        }
        else if (channel.path == AnimationPath::Scale)
        {
            glm::vec3 s0 = channel.keyframeScales[previous_index];
            glm::vec3 s1 = channel.keyframeScales[next_index];
            node.scale = glm::mix(s0, s1, factor); // Linear interpolation
        }

        // Rebuild local matrix
        node.localMatrix = glm::translate(glm::mat4(1.0f), node.translation) *
                           glm::mat4_cast(node.rotation) *
                           glm::scale(glm::mat4(1.0f), node.scale);
    }

    // Recursively update world matrices down the tree now that local transforms changed
    updateAllMatrices();
}

void CPUModel::computeJointMatrices(const int nodeIndex, std::vector<glm::mat4>& matrices) const
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes.size()))
    {
        return;
    }

    const CPUNode& node = nodes[nodeIndex];

    // If this node not have a skin, we do not neet joint matrices
    if (node.skinIndex < 0 || node.skinIndex >= static_cast<int>(skins.size()))
    {
        std::cout << "[Anim Error] Node skin index is invalid!" << std::endl;
        matrices.clear();
        return;
    }

    const CPUSkin& skin = skins[node.skinIndex];
    matrices.resize(skin.jointNodes.size());

    // glTF spec: JointMatrix = inverse(MeshWorld) * JointWorld * InverseBind
    glm::mat4 inverse_mesh_transform = glm::inverse(node.worldMatrix);
    for (size_t joint_nodes_index = 0; joint_nodes_index < skin.jointNodes.size(); ++joint_nodes_index)
    {
        const CPUNode& joint_node = nodes[skin.jointNodes[joint_nodes_index]];
        matrices[joint_nodes_index] = inverse_mesh_transform * joint_node.worldMatrix * skin.inverseBindMatrices[joint_nodes_index];
    }
}

namespace ModelLoader
{
    void UpdateAllMatrices(CPUModel& cpuModel)
    {
        for (const int rootNodeIndex : cpuModel.sceneRootNodes)
        {
            ComputeWorldMatrices(cpuModel, rootNodeIndex, glm::mat4(1.0f));
        }
    }

    CPUModel LoadGLTF(const std::string& filePath)
    {
        CPUModel cpu_model;
        cpu_model.loadGLTF(filePath);
        return cpu_model;
    }

    // Only returns CPU data
    CPUMesh_Single LoadSingleMeshGLTF(const std::string& filePath)
    {
        CPUModel model = LoadGLTF(filePath);

        if (model.meshes.empty())
        {
            return {};
        }

        CPUMesh_Single returnMesh;
        CPUMesh internalCPUMesh= model.meshes[0];

        if (!internalCPUMesh.primitives.empty())
        {
            returnMesh.vertices = internalCPUMesh.primitives[0].vertices;
            returnMesh.indices = internalCPUMesh.primitives[0].indices;

            const int materialIndex = internalCPUMesh.primitives[0].materialIndex;
            if (materialIndex >= 0 && materialIndex < static_cast<int>(model.materials.size()))
            {
                const int textureIndex = model.materials[materialIndex].baseColorTextureIndex;
                if (textureIndex >= 0 && textureIndex < static_cast<int>(model.textures.size()))
                {
                    returnMesh.texturePath = model.textures[textureIndex].imagePath;
                }
            }
        }

        return returnMesh;
    }
} //AnvilModelLoader

namespace
{
    std::string GetBaseDirectory(const std::string& file_path)
    {
        const std::filesystem::path path(file_path);
        const std::filesystem::path parent = path.parent_path();

        if (parent.empty())
        {
            return "";
        }

        return parent.string() + std::string(1, std::filesystem::path::preferred_separator);
    }

    std::string JoinPath(const std::string& base_directory, const char* uri)
    {
        if (!uri)
        {
            return {};
        }

        const std::filesystem::path uri_path(uri);
        if (uri_path.is_absolute())
        {
            return uri_path.string();
        }

        return (std::filesystem::path(base_directory) / uri_path).string();
    }

    void LoadTextures(CPUModel& model, const cgltf_data* gltf_data, const std::string& base_directory)
    {
        for (cgltf_size texture_index = 0; texture_index < gltf_data->textures_count; ++texture_index)
        {
            CPUTexture& cpu_texture = model.textures[texture_index];
            const cgltf_texture& gltf_texture = gltf_data->textures[texture_index];

            cpu_texture.name = gltf_texture.name ? gltf_texture.name : ("Texture_" + std::to_string(texture_index));

            if (gltf_texture.image && gltf_texture.image->uri)
            {
                cpu_texture.imagePath = JoinPath(base_directory, gltf_texture.image->uri);
            }

            // model.textures.push_back(cpu_texture);
        }
    }

    void LoadMaterials(CPUModel& model, const cgltf_data* gltf_data)
    {
        for (cgltf_size material_index = 0; material_index < gltf_data->materials_count; ++material_index)
        {
            CPUMaterial& cpu_material = model.materials[material_index];
            const cgltf_material& gltf_material = gltf_data->materials[material_index];

            cpu_material.name = gltf_material.name ? gltf_material.name : "Material_" + std::to_string(material_index);
            cpu_material.alphaCutoff = static_cast<float>(gltf_material.alpha_cutoff);

            if (gltf_material.has_pbr_metallic_roughness)
            {
                const cgltf_pbr_metallic_roughness& pbr = gltf_material.pbr_metallic_roughness;

                cpu_material.baseColorFactor = glm::vec4(
                    static_cast<float>(pbr.base_color_factor[0]),
                    static_cast<float>(pbr.base_color_factor[1]),
                    static_cast<float>(pbr.base_color_factor[2]),
                    static_cast<float>(pbr.base_color_factor[3])
                );

                cpu_material.baseColorTextureIndex = GetTextureIndex(gltf_data, pbr.base_color_texture.texture);
                cpu_material.metallicRoughnessTextureIndex = GetTextureIndex(gltf_data, pbr.metallic_roughness_texture.texture);
                cpu_material.metallicFactor = static_cast<float>(pbr.metallic_factor);
                cpu_material.roughnessFactor = static_cast<float>(pbr.roughness_factor);
            }

            if (gltf_material.normal_texture.texture)
            {
                cpu_material.normalTextureIndex = GetTextureIndex(gltf_data, gltf_material.normal_texture.texture);
            }
        }
    }

    void LoadMeshes(CPUModel& cpu_model, const cgltf_data* gltf_data)
    {
        for (cgltf_size mesh_index = 0; mesh_index < gltf_data->meshes_count; ++mesh_index)
        {
            CPUMesh& cpu_mesh = cpu_model.meshes[mesh_index];
            const cgltf_mesh& gltf_mesh = gltf_data->meshes[mesh_index];

            cpu_mesh.name = gltf_mesh.name ? gltf_mesh.name : "Mesh_" + std::to_string(mesh_index);

            cpu_mesh.primitives.resize(gltf_mesh.primitives_count);
            for (cgltf_size primitive_index = 0; primitive_index < gltf_mesh.primitives_count; ++primitive_index)
            {
                CPUMeshPrimitive& cpu_mesh_primitive = cpu_mesh.primitives[primitive_index];
                const cgltf_primitive& gltf_primitive = gltf_mesh.primitives[primitive_index];

                if (gltf_primitive.type != cgltf_primitive_type_triangles)
                {
                    std::cerr << "Skipping non-triangle glTF primitive in mesh: " << cpu_mesh.name << std::endl;
                    continue;
                }

                const cgltf_accessor* position_accessor = nullptr;
                const cgltf_accessor* normal_accessor = nullptr;
                const cgltf_accessor* uv_accessor = nullptr;

                // Pointers for skinning accessors
                const cgltf_accessor* joints_accessor = nullptr;
                const cgltf_accessor* weights_accessor = nullptr;

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
                    else if (attribute.type == cgltf_attribute_type_joints)
                    {
                        joints_accessor = attribute.data;
                    }
                    else if (attribute.type == cgltf_attribute_type_weights)
                    {
                        weights_accessor = attribute.data;
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

                    if (uv_accessor)
                    {
                        cgltf_accessor_read_float(uv_accessor, vertex_index, &vertex.uv.x, 2);
                    }

                    if (joints_accessor)
                    {
                        cgltf_accessor_read_uint(joints_accessor, vertex_index, &vertex.joints.x, 4);
                    }

                    if (weights_accessor)
                    {
                        cgltf_accessor_read_float(weights_accessor, vertex_index, &vertex.weights.x, 4);
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

                // cpu_mesh.primitives.push_back(std::move(cpu_mesh_primitive));
            }
            // cpu_model.meshes.push_back(std::move(cpu_mesh));
        }
    }

    void LoadNodes(CPUModel& cpu_model, const cgltf_data* gltf_data)
    {
        for (cgltf_size node_index = 0; node_index < gltf_data->nodes_count; ++node_index)
        {
            CPUNode& cpu_node = cpu_model.nodes[node_index];
            const cgltf_node& gltf_node = gltf_data->nodes[node_index];

            cpu_node.name = gltf_node.name ? gltf_node.name : "Node_" + std::to_string(node_index);

            cpu_node.meshIndex = GetMeshIndex(gltf_data, gltf_node.mesh);
            cpu_node.skinIndex = GetSkinIndex(gltf_data, gltf_node.skin);
            cpu_node.parentIndex = GetNodeIndex(gltf_data, gltf_node.parent);
            ReadNodeTRS(cpu_node, &gltf_node);

            if (gltf_node.has_matrix)
            {
                cpu_node.localMatrix = ConvertMatrix(gltf_node.matrix);
            }
            else
            {
                cpu_node.localMatrix = MakeLocalMatrix(cpu_node);
            }

            cpu_node.worldMatrix = cpu_node.localMatrix;
        }

        // Node children
        for (cgltf_size node_index = 0; node_index < gltf_data->nodes_count; ++node_index)
        {
            const cgltf_node& gltf_node = gltf_data->nodes[node_index];

            for (cgltf_size child_index = 0; child_index < gltf_node.children_count; ++child_index)
            {
                const int model_child_index = GetNodeIndex(gltf_data, gltf_node.children[child_index]);

                if (model_child_index >= 0)
                {
                    cpu_model.nodes[node_index].children.push_back(model_child_index);
                }
            }
        }
    }

    void LoadSkins(CPUModel& cpu_model, const cgltf_data* gltf_data)
    {
        for (cgltf_size skin_index = 0; skin_index < gltf_data->skins_count; ++skin_index)
        {
            CPUSkin &cpu_skin = cpu_model.skins[skin_index];
            const cgltf_skin& gltf_skin = gltf_data->skins[skin_index];

            cpu_skin.name = gltf_skin.name ? gltf_skin.name : "Skin_" + std::to_string(skin_index);

            if (gltf_skin.skeleton)
            {
                cpu_skin.skeletonRootNode = GetNodeIndex(gltf_data, gltf_skin.skeleton);
            }

            cpu_skin.jointNodes.resize(gltf_skin.joints_count);
            for (cgltf_size joint_index = 0; joint_index < gltf_skin.joints_count; ++joint_index)
            {
                cpu_skin.jointNodes[joint_index] = GetNodeIndex(gltf_data, gltf_skin.joints[joint_index]);
            }

            if (gltf_skin.inverse_bind_matrices)
            {
                cpu_skin.inverseBindMatrices.resize(gltf_skin.joints_count);
                for (cgltf_size joint_index = 0; joint_index < gltf_skin.joints_count; ++joint_index)
                {
                    cgltf_float inverse_bind[16];
                    cgltf_accessor_read_float(gltf_skin.inverse_bind_matrices, joint_index, inverse_bind, 16);
                    cpu_skin.inverseBindMatrices[joint_index] = ConvertMatrix(inverse_bind);
                }
            }
            else
            {
                cpu_skin.inverseBindMatrices.resize(gltf_skin.joints_count, glm::mat4(1.0f));
            }
        }
    }

    void LoadAnimations(CPUModel& cpu_model, const cgltf_data* gltf_data)
    {
        for (cgltf_size animation_index = 0; animation_index < gltf_data->animations_count; ++animation_index)
        {
            CPUAnimation& cpu_animation = cpu_model.animations[animation_index];
            const cgltf_animation& gltf_animation = gltf_data->animations[animation_index];
            cpu_animation.name = gltf_animation.name ? gltf_animation.name : ("Animation_" + std::to_string(animation_index));

            cpu_animation.channels.resize(gltf_animation.channels_count);
            for (cgltf_size channel_index = 0; channel_index < gltf_animation.channels_count; ++channel_index)
            {
                CPUAnimationChannel& cpu_channel = cpu_animation.channels[channel_index];
                const cgltf_animation_channel& gltf_channel = gltf_animation.channels[channel_index];

                cpu_channel.targetNodeIndex = GetNodeIndex(gltf_data, gltf_channel.target_node);

                if (cpu_channel.targetNodeIndex < 0 || !gltf_channel.sampler)
                {
                    continue;
                }

                // Determine the target path
                if (gltf_channel.target_path == cgltf_animation_path_type_translation)
                {
                    cpu_channel.path = AnimationPath::Translation;
                }
                else if (gltf_channel.target_path == cgltf_animation_path_type_rotation)
                {
                    cpu_channel.path = AnimationPath::Rotation;
                }
                else if (gltf_channel.target_path == cgltf_animation_path_type_scale)
                {
                    cpu_channel.path = AnimationPath::Scale;
                }
                else
                {
                    // Skip weights/morphs for now
                    continue;
                }

                const cgltf_accessor* input_accessor = gltf_channel.sampler->input;
                const cgltf_accessor* output_accessor = gltf_channel.sampler->output;

                // Extract Keyframe Times
                cpu_channel.keyframeTimes.resize(input_accessor->count);
                for (cgltf_size time_index = 0; time_index < input_accessor->count; ++time_index)
                {
                    cgltf_accessor_read_float(input_accessor, time_index, &cpu_channel.keyframeTimes[time_index], 1);
                    cpu_animation.duration = std::max(cpu_animation.duration, cpu_channel.keyframeTimes[time_index]);
                }

                if (cpu_channel.path == AnimationPath::Rotation)
                {
                    // Extract Keyframe Rotations
                    cpu_channel.keyframeRotations.resize(output_accessor->count);
                    for (cgltf_size rotation_index = 0; rotation_index < output_accessor->count; ++rotation_index)
                    {
                        float rotation[4]; // gltf quaternions as [x, y, z, w]
                        cgltf_accessor_read_float(output_accessor, rotation_index, rotation, 4);
                        // glm::quat expects [w, x, y, z]
                        cpu_channel.keyframeRotations[rotation_index] = glm::quat(rotation[3], rotation[0], rotation[1], rotation[2]);
                    }
                }
                else if (cpu_channel.path == AnimationPath::Translation)
                {
                    cpu_channel.keyframeTranslations.resize(output_accessor->count);
                    for (cgltf_size translation_index = 0; translation_index < output_accessor->count; ++translation_index)
                    {
                        auto& translations = cpu_channel.keyframeTranslations[translation_index];
                        cgltf_accessor_read_float(output_accessor, translation_index, &translations.x, 3);
                    }
                }
                else if (cpu_channel.path == AnimationPath::Scale)
                {
                    cpu_channel.keyframeScales.resize(output_accessor->count);
                    for (cgltf_size scale_index = 0; scale_index < output_accessor->count; ++scale_index)
                    {
                        auto& scales = cpu_channel.keyframeScales[scale_index];
                        cgltf_accessor_read_float(output_accessor, scale_index, &scales.x, 3);
                    }
                }

                // cpu_animation.channels.push_back(std::move(cpu_channel));
            }
            // cpu_model.animations.push_back(std::move(cpu_animation));
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

    int GetSkinIndex(const cgltf_data* data, const cgltf_skin* skin)
    {
        if (!skin) return -1;
        for (cgltf_size i = 0; i < data->skins_count; ++i)
        {
            if (&data->skins[i] == skin) return static_cast<int>(i);
        }
        return -1;
    }

    glm::mat4 ConvertMatrix(const cgltf_float* cgltf_matrix)
    {
        glm::mat4 result(1.0f);

        // cgltf stores the glTF matrix values in column-major order.
        for (int column = 0; column < 4; ++column)
        {
            for (int row = 0; row < 4; ++row)
            {
                result[column][row] = cgltf_matrix[column * 4 + row];
            }
        }

        return result;
    }

    glm::mat4 MakeLocalMatrix(const CPUNode& cpu_node)
    {
        const glm::mat4 translation = glm::translate(glm::mat4(1.0f), cpu_node.translation);
        const glm::mat4 rotation = glm::mat4_cast(cpu_node.rotation);
        const glm::mat4 scale = glm::scale(glm::mat4(1.0f), cpu_node.scale);

        return translation * rotation * scale;
    }

    void ReadNodeTRS(CPUNode& cpu_node, const cgltf_node* gltf_node)
    {
        if (gltf_node->has_translation)
        {
            cpu_node.translation = glm::vec3(
                static_cast<float>(gltf_node->translation[0]),
                static_cast<float>(gltf_node->translation[1]),
                static_cast<float>(gltf_node->translation[2])
            );
        }

        if (gltf_node->has_rotation)
        {
            cpu_node.rotation = glm::quat(
                static_cast<float>(gltf_node->rotation[3]),
                static_cast<float>(gltf_node->rotation[0]),
                static_cast<float>(gltf_node->rotation[1]),
                static_cast<float>(gltf_node->rotation[2])
            );
        }

        if (gltf_node->has_scale)
        {
            cpu_node.scale = glm::vec3(
                static_cast<float>(gltf_node->scale[0]),
                static_cast<float>(gltf_node->scale[1]),
                static_cast<float>(gltf_node->scale[2])
            );
        }
    }

    void ComputeWorldMatrices(CPUModel& cpu_model, const int node_index, const glm::mat4& parent_matrix)
    {
        if (node_index < 0 || node_index >= static_cast<int>(cpu_model.nodes.size()))
        {
            return;
        }

        CPUNode& node = cpu_model.nodes[node_index];
        node.worldMatrix = parent_matrix * node.localMatrix;

        for (const int child_index : node.children)
        {
            // Some recursion, hopefully does not causes crashes
            ComputeWorldMatrices(cpu_model, child_index, node.worldMatrix);
        }
    }
}
