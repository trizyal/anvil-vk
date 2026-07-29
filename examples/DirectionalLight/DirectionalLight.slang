// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

struct PushConstants
{
    float4x4 renderMatrix;
    float4x4 modelMatrix;
}

// [[vk::push_constant]] tells Slang to map this to Vulkan Push Constants
[[vk::push_constant]]
ConstantBuffer<PushConstants> pushConstants;

// A new Uniform Buffer for Lighting
struct SceneData
{
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
};

// Reflection should automatically map this to a Uniform Buffer binding
ConstantBuffer<SceneData> sceneBuffer;

Sampler2D texture; // Do I need [[vk::binding(0, 0)]]

// Vertex Input also needs glTF Normals
struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL; // AnvilMeshBuffer needs to provide this
    float2 uv       : TEXCOORD;
};

struct VertexOutput
{
    float3 position     : SV_Position;
    float4 worldNormal  : NORMAL;
    float2 uv           : TEXCOORD;
};
