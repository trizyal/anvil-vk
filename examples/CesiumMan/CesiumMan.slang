// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

// ==========================================
// VERTEX INPUT / OUTPUT
// ==========================================

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
    float4 position     : SV_Position;
    float3 worldNormal  : NORMAL;
    float2 uv           : TEXCOORD;
};

// ==========================================
// BUFFERS & PUSH CONSTANTS
// ==========================================

struct PushConstants
{
    float4x4 renderMatrix;
    float4x4 modelMatrix;
    float4 cameraPosition;
    float4 baseColorFactor;
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
[[vk::binding(0, 1)]] StructuredBuffer<float4x4> jointMatrices;

// SET 2: Material
[[vk::binding(0, 2)]] Sampler2D baseColorTexture;

// ==========================================
// VERTEX SHADER
// ==========================================

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    // Default to Identity Matrix if unrigged
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

    // Apply skinning on the CPU/local space before world transform
    float4 localPos = mul(skinMat, float4(input.position, 1.0));
    float3 localNormal = mul((float3x3)skinMat, input.normal);

    VertexOutput output;
    output.position = mul(pushConstants.renderMatrix, localPos);
    output.worldNormal = mul((float3x3)pushConstants.modelMatrix, localNormal);
    output.uv = input.uv;

    return output;
}

// ==========================================
// FRAGMENT SHADER
// ==========================================

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    // Sample the texture and multiply by the material base color factor
    float4 albedo = baseColorTexture.Sample(input.uv) * pushConstants.baseColorFactor;

    // Directional lighting calculation
    float3 normal = normalize(input.worldNormal);
    float3 lightDir = normalize(-sceneBuffer.lightDirection.xyz);

    float nDotL = max(dot(normal, lightDir), 0.0);

    float3 diffuse = albedo.rgb * sceneBuffer.lightColor.rgb * nDotL;
    float3 ambient = albedo.rgb * sceneBuffer.ambientColor.rgb;

    return float4(diffuse + ambient, albedo.a);
}