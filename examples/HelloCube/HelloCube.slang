// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

struct PushConstants
{
    float4x4 renderMatrix;
};

// [[vk::push_constant]] tells Slang to map this to Vulkan Push Constants
[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants;

// SET 0: Scene - Let's use this to tint the cube to prove we correctly attached the Scene module!
struct SceneData
{
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
};

[[vk::binding(0, 0)]]
ConstantBuffer<SceneData> sceneBuffer;

struct VertexInput
{
    float3 position : POSITION;
    float3 color    : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;

    output.position = mul(pushConstants.renderMatrix, float4(input.position, 1.0));
    output.color = input.color;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    // Use the scene buffer data to tint the vertex color
    float3 tintedColor = input.color * sceneBuffer.lightColor.rgb;

    return float4(tintedColor, 1.0);
}