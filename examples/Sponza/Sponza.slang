// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
};

struct VertexOutput
{
    float4 position     : SV_Position;
    float3 worldNormal  : NORMAL;
    float2 uv           : TEXCOORD;
};

struct PushConstants
{
    float4x4 renderMatrix;
    float4x4 modelMatrix;
    float4 cameraPosition;
};

struct SceneData
{
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
};

// SET 0: Scene
[[vk::binding(0, 0)]] ConstantBuffer<SceneData> sceneBuffer;

// SET 1: Model
[[vk::binding(0, 1)]] // Unused as no joints or skinning

// SET 2: Material
[[vk::binding(0, 2)]] Sampler2D baseColorTexture;
[[vk::binding(1, 2)]] Sampler2D normalTexture;
