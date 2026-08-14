// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

struct PushConstants
{
    float4x4 renderMatrix; // Projection * View * Model
    float4x4 modelMatrix;  // Model rotation for world-space normals
    float4 camera;
    float4 baseColorFactor;
};

// A new Uniform Buffer for Lighting
struct SceneData
{
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
};

// Reflection should automatically map this to a Uniform Buffer binding
[[vk::binding(0, 0)]]
ConstantBuffer<SceneData> sceneBuffer;

[[vk::binding(1, 0)]]
Sampler2D texture;

[[vk::push_constant]] // Tells Slang to map this to Vulkan Push Constants
ConstantBuffer<PushConstants> pushConstants;

struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
};

struct VertexOutput
{
    float4 position      : SV_Position;
    float3 worldNormal   : NORMAL;
    float2 uv            : TEXCOORD;
    float3 worldPosition : POSITION0;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;

    float4 worldPosition = mul(pushConstants.modelMatrix, float4(input.position, 1.0));

    // Transform position to clip space
    output.position = mul(float4(input.position, 1), pushConstants.renderMatrix);

    // Rotate normal to world space (w = 0.0 ignores translation)
    output.worldNormal = mul(float4(input.normal, 0.0), pushConstants.modelMatrix).xyz;

    output.uv = input.uv;
    output.worldPosition = worldPosition.xyz;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float4 textureColor = texture.Sample(input.uv);
    float4 baseColor = textureColor * pushConstants.baseColorFactor;

    float3 normal = normalize(input.worldNormal);
    float3 lightDirection = normalize(-sceneBuffer.lightDirection.xyz); // Arrow pointing AT the light source

    float nDotL = max(dot(normal, lightDirection), 0.0);

    float3 ambient = sceneBuffer.ambientColor.rgb;
    float3 diffuse = sceneBuffer.lightColor.rgb * sceneBuffer.lightColor.w * nDotL;

    float3 finalLighting = baseColor.rgb * (ambient + diffuse);

    return float4(finalLighting, baseColor.a);
}
