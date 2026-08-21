// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

struct PushConstants
{
    float4x4 renderMatrix; // Projection * View * Model
    float4x4 modelMatrix;  // Model rotation for world-space normals
    float4 camera;
    float4 baseColorFactor;
};

[[vk::push_constant]] // Tells Slang to map this to Vulkan Push Constants
ConstantBuffer<PushConstants> pushConstants;

[[vk::binding(2, 0)]]
StructuredBuffer<column_major float4x4> jointMatrices;

struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
    uint4 joints    : JOINTS; // The 4 bones affecting this vector
    float4 weights  : WEIGHTS; // The influence weight of each bone
};

struct VertexOutput
{
    float4 position      : SV_Position;
    float3 worldNormal   : NORMAL;
    float2 uv            : TEXCOORD;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    // Start with an Identity Matrix (No bending)
    float4x4 skinMat = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    // If the total weight is > 0, this mesh is rigged to the skeleton.
    float totalWeight = input.weights.x + input.weights.y + input.weights.z + input.weights.w;

    if (totalWeight > 0.0)
    {
        // Multiply each bone's matrix by its weight, and sum them together
        skinMat =
            input.weights.x * jointMatrices[input.joints.x] +
            input.weights.y * jointMatrices[input.joints.y] +
            input.weights.z * jointMatrices[input.joints.z] +
            input.weights.w * jointMatrices[input.joints.w];
    }

    // Apply the Skin Matrix to the local position and normal BEFORE applying the world matrix
    float4 localPos = mul(skinMat, float4(input.position, 1.0));

    // Cast to float3x3 to rotate the normal without translating it
    float3 localNormal = mul((float3x3)skinMat, input.normal);

    VertexOutput output;

    // Transform position to clip space
    output.position = mul(pushConstants.renderMatrix, localPos);

    // Rotate normal to world space (w = 0.0 ignores translation)
    output.worldNormal = mul((float3x3)pushConstants.modelMatrix, localNormal);

    output.uv = input.uv;
    return output;
}

// A new Uniform Buffer for Lighting
struct SceneData
{
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
};

// Reflection should automatically map this to a Uniform Buffer binding
[[vk::binding(1, 0)]]
ConstantBuffer<SceneData> sceneBuffer;

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float4 baseColor = pushConstants.baseColorFactor;

    float3 normal = normalize(input.worldNormal);
    float3 lightDirection = normalize(-sceneBuffer.lightDirection.xyz); // Arrow pointing AT the light source

    float nDotL = max(dot(normal, lightDirection), 0.0);

    float3 ambient = sceneBuffer.ambientColor.rgb;
    float3 diffuse = sceneBuffer.lightColor.rgb * sceneBuffer.lightColor.w * nDotL;

    float3 finalLighting = baseColor.rgb * (ambient + diffuse);

    return float4(finalLighting, baseColor.a);
}
