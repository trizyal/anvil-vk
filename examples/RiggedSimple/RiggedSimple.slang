// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

struct PushConstants
{
    float4x4 renderMatrix; // Projection * View * Model
    float4x4 modelMatrix;  // Model rotation for world-space normals
    float4 camera;
    float4 baseColorFactor;
};

[[vk::push_constant]]
ConstantBuffer<PushConstants> pushConstants;

// SET 0: Scene
struct SceneData
{
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
};

[[vk::binding(0, 0)]]
ConstantBuffer<SceneData> sceneBuffer;

// SET 1: Model
[[vk::binding(0, 1)]]
StructuredBuffer<column_major float4x4> jointMatrices;

struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
    uint4 joints    : JOINTS;
    float4 weights  : WEIGHTS;
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
    float4x4 skinMat = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    };

    float totalWeight = input.weights.x + input.weights.y + input.weights.z + input.weights.w;

    if (totalWeight > 0.0)
    {
        skinMat =
            input.weights.x * jointMatrices[input.joints.x] +
            input.weights.y * jointMatrices[input.joints.y] +
            input.weights.z * jointMatrices[input.joints.z] +
            input.weights.w * jointMatrices[input.joints.w];
    }

    float4 localPos = mul(skinMat, float4(input.position, 1.0));
    float3 localNormal = mul((float3x3)skinMat, input.normal);

    VertexOutput output;
    output.position = mul(pushConstants.renderMatrix, localPos);
    output.worldNormal = mul((float3x3)pushConstants.modelMatrix, localNormal);
    output.uv = input.uv;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float4 baseColor = pushConstants.baseColorFactor;

    float3 normal = normalize(input.worldNormal);
    float3 lightDirection = normalize(-sceneBuffer.lightDirection.xyz);

    float nDotL = max(dot(normal, lightDirection), 0.0);

    float3 ambient = sceneBuffer.ambientColor.rgb;
    float3 diffuse = sceneBuffer.lightColor.rgb * sceneBuffer.lightColor.w * nDotL;

    float3 finalLighting = baseColor.rgb * (ambient + diffuse);

    return float4(finalLighting, baseColor.a);
}