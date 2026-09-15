// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "shared/DebugModes.h"
#include "shared/PushConstants.h"

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

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants;
[[vk::binding(0, 1)]] StructuredBuffer<float4x4> modelMatrices;
[[vk::binding(0, 2)]] Sampler2D baseColorTexture;
[[vk::binding(1, 2)]] Sampler2D normalTexture;
[[vk::binding(2, 2)]] Sampler2D metallicRoughnessTexture;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input) {
    VertexOutput output;
    float4x4 modelMatrix = modelMatrices[pushConstants.objectIndex];
    float4 worldPos = mul(modelMatrix, float4(input.position, 1.0));

    output.worldPosition = worldPos.xyz;
    output.position = mul(pushConstants.viewProjection, worldPos);
    output.worldNormal = mul((float3x3)modelMatrix, input.normal);
    output.worldTangent = float4(mul((float3x3)modelMatrix, input.tangent.xyz), input.tangent.w);
    output.uv = input.uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target {
    float4 albedo = baseColorTexture.Sample(input.uv);
    if (albedo.a < 0.5) discard;

    float3 geoNormal = normalize(input.worldNormal);
    float3 T = normalize(input.worldTangent.xyz);
    T = normalize(T - dot(T, geoNormal) * geoNormal);
    float3 B = cross(geoNormal, T) * input.worldTangent.w;
    float3x3 TBN = float3x3(T, B, geoNormal);

    float3 sampledNormal = normalTexture.Sample(input.uv).xyz;
    float3 mappedNormal = normalize(mul(sampledNormal* 2.0 - 1.0, TBN));

    // Sample PBR
    float4 metRough = metallicRoughnessTexture.Sample(input.uv);
    float roughness = metRough.g;
    float metallic = metRough.b;
    float depth = distance(pushConstants.cameraPosition.xyz, input.worldPosition) / 50.0f;

    switch (pushConstants.debugMode)
    {
        case DebugMode::BaseColor:
            return albedo;
        case DebugMode::GeometryNormal:
            return float4(geoNormal * 0.5 + 0.5, 1.0);
        case DebugMode::RawNormalMap:
            return float4(sampledNormal, 1.0);
        case DebugMode::WorldNormal:
            return float4(mappedNormal * 0.5 + 0.5, 1.0);
        case DebugMode::Metallic:
            return float4(metallic, metallic, metallic, 1.0);
        case DebugMode::Roughness:
            return float4(roughness, roughness, roughness, 1.0);
        case DebugMode::Depth:
            return float4(depth, depth, depth, 1.0);
        case DebugMode::Overdraw:
            return float4(0.05, 0.20, 0.05, 1.0); // Overdraw
        case DebugMode::Overshading:
            return float4(0.05, 0.20, 0.05, 1.0); // Overshading
        default: return float4(0.0);
    }
}
