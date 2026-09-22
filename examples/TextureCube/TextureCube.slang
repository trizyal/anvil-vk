// ==========================================
// TextureCube.slang
// ==========================================

// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

struct PushConstants
{
    float4x4 renderMatrix; // Projection * View * Model
    float4x4 modelMatrix;  // Model rotation for world-space normals
    float4 baseColorFactor;
};

// [[vk::push_constant]] tells Slang to map this to Vulkan Push Constants
[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants;

// SET 2: Material (Matches GPUModel architecture)
[[vk::binding(0, 2)]]
Sampler2D baseColorTexture;

struct VertexInput
{
    float3 position : POSITION;
    float2 uv       : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;

    output.position = mul(pushConstants.renderMatrix, float4(input.position, 1.0));
    output.uv = input.uv;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    // Sample the standard Set 2 texture
    float4 textureColor = baseColorTexture.Sample(input.uv);

    return textureColor * pushConstants.baseColorFactor;
}