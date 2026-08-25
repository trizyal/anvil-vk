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

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants;

// SET 0: Scene
[[vk::binding(0, 0)]] ConstantBuffer<SceneData> sceneBuffer;

// SET 1: Model
// [[vk::binding(0, 1)]] // Unused as no joints or skinning

// SET 2: Material
[[vk::binding(0, 2)]] Sampler2D baseColorTexture;
[[vk::binding(1, 2)]] Sampler2D normalTexture;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(pushConstants.renderMatrix, float4(input.position, 1.0));
    output.worldNormal = mul((float3x3)pushConstants.modelMatrix, input.normal);
    output.uv = input.uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float4 albedo = baseColorTexture.Sample(input.uv);

    // Alpha test for Sponza's foliage/flags
    if (albedo.a < 0.5)
    {
        discard;
    }

    float3 normal = normalize(input.worldNormal);
    float3 lightDir = normalize(-sceneBuffer.lightDirection.xyz);

    float nDotL = max(dot(normal, lightDir), 0.0);

    float3 diffuse = albedo.rgb * sceneBuffer.lightColor.rgb * nDotL;
    float3 ambient = albedo.rgb * sceneBuffer.ambientColor.rgb;

    return float4(diffuse + ambient, albedo.a);
}
