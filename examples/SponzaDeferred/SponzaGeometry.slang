// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

struct VertexInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float4 tangent  : TANGENT;
    float2 uv       : TEXCOORD;
};

struct VertexOutput
{
    float4 position         : SV_Position;
    float3 worldPosition    : POSITION;
    float3 worldNormal      : NORMAL;
    float4 worldTangent     : TANGENT;
    float2 uv               : TEXCOORD;
};

struct PushConstants
{
    float4x4 viewProjection;
    float4 cameraPosition;
    uint32_t objectIndex;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants;

// SET 0: Scene
// [[vk::binding(0, 0)]] ConstantBuffer<SceneData> sceneBuffer;

// SET 1: Model
// [[vk::binding(0, 1)]] [[vk::binding(0, 1)]] StructuredBuffer<float4x4> jointMatrices;
[[vk::binding(0, 1)]] StructuredBuffer<float4x4> modelMatrices;

// SET 2: Material
[[vk::binding(0, 2)]] Sampler2D baseColorTexture;
[[vk::binding(1, 2)]] Sampler2D normalTexture;
[[vk::binding(2, 2)]] Sampler2D metallicRoughnessTexture;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;

    // Fetch matrix from SSBO
    float4x4 modelMatrix = modelMatrices[pushConstants.objectIndex];

    float4 worldPos = mul(modelMatrix, float4(input.position, 1.0));
    output.worldPosition = worldPos.xyz;
    output.position = mul(pushConstants.viewProjection, worldPos);

    output.worldNormal = mul((float3x3)modelMatrix, input.normal);
    output.worldTangent = float4(mul((float3x3)modelMatrix, input.tangent.xyz), input.tangent.w);
    output.uv = input.uv;
    return output;
}

struct FragmentOutput
{
    float4 albedo           : SV_Target0;
    float4 normal           : SV_Target1;
    float4 pbr              : SV_Target2;
    float4 worldPosition    : SV_Target3;
};

[shader("fragment")]
FragmentOutput fragmentMain(VertexOutput input)
{
    FragmentOutput output;

    output.albedo = baseColorTexture.Sample(input.uv);
    if (output.albedo.a < 0.5) discard; // Alpha test

    // glTF PBR standard: B channel = Metallic, G channel = Roughness
    output.pbr  = metallicRoughnessTexture.Sample(input.uv);

    // Build the Tangent-Bitangent-Normal (TBN) Matrix
    float3 N = normalize(input.worldNormal);
    float3 T = normalize(input.worldTangent.xyz);
    T = normalize(T - dot(T, N) * N); // Re-orthogonalize T with respect to N
    float3 B = cross(N, T) * input.worldTangent.w;
    float3x3 TBN = float3x3(T, B, N);

    float3 sampledNormal = normalTexture.Sample(input.uv).xyz * 2.0 - 1.0;
    output.normal = float4(normalize(mul(sampledNormal, TBN)), 1.0f);
    output.worldPosition = float4(input.worldPosition, 1.0);

    return output;
}
