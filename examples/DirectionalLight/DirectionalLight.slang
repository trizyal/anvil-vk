// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

struct PushConstants
{
    float4x4 renderMatrix; // Projection * View * Model
    float4x4 modelMatrix;  // Model rotation for world-space normals
    float4 camera;
    float4 baseColorFactor;
}

// [[vk::push_constant]] tells Slang to map this to Vulkan Push Constants
[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants;

// A new Uniform Buffer for Lighting
struct SceneData
{
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
};

// SET 0: Scene
[[vk::binding(0, 0)]]
ConstantBuffer<SceneData> sceneBuffer;

// SET 2: Material
[[vk::binding(0, 2)]]
Sampler2D baseColorTexture;

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

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;

    // Transform position to clip space
    output.position = mul(pushConstants.renderMatrix, float4(input.position, 1));

    // Rotate normal to world space (w = 0.0 ignores translation)
    output.worldNormal = mul(pushConstants.modelMatrix, float4(input.normal, 0.0)).xyz;

    output.uv = input.uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float3 normal = normalize(input.worldNormal);
    float3 lightDirection = normalize(-sceneBuffer.lightDirection.xyz); // Arrow pointing AT the light source

    // Calculate Diffuse (N dot L)
    float diff = max(dot(normal, lightDirection), 0.0);
    float3 diffuse = diff * sceneBuffer.lightColor.xyz * sceneBuffer.lightColor.w;

    // Combine Ambient + Diffuse
    float3 finalLighting = sceneBuffer.ambientColor.xyz + diffuse;

    // Sample base texture and multiply by lighting & push constants
    float4 textureColor = baseColorTexture.Sample(input.uv);
    float4 baseColor = textureColor * pushConstants.baseColorFactor;

    return float4(baseColor.rgb * finalLighting, baseColor.a);
}