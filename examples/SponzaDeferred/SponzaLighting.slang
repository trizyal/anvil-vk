// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

import DebugViews;

struct PushConstants
{
    float4x4 viewProjection;
    float4 cameraPosition;
    uint32_t objectIndex;
};

struct SceneData
{
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
    uint debugMode;
    float3 _padding;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants;

[[vk::binding(0, 0)]] ConstantBuffer<SceneData> sceneBuffer;

[[vk::binding(1, 0)]] Sampler2D gAlbedo;
[[vk::binding(2, 0)]] Sampler2D gNormal;
[[vk::binding(3, 0)]] Sampler2D gPBR;
[[vk::binding(4, 0)]] Sampler2D gWorldPosition;

[shader("vertex")]
float4 vertexMain(uint vertexID : SV_VertexID) : SV_Position
{
    // Generate fullscreen triangle procedurally
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    return float4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
}

[shader("fragment")]
float4 fragmentMain(float4 pos : SV_Position) : SV_Target
{
    // Reconstruct UV from pixel coordinates
    int2 texelCoord = int2(pos.xy);
    float4 albedo = gAlbedo.Load(int3(texelCoord, 0));
    float3 normal = gNormal.Load(int3(texelCoord, 0)).xyz;
    float4 pbr    = gPBR.Load(int3(texelCoord, 0));
    float3 wPos   = gWorldPosition.Load(int3(texelCoord, 0)).xyz;

    float roughness = pbr.g;
    float metallic  = pbr.b;

    if (sceneBuffer.debugMode != 0) return GetDebugColor(sceneBuffer.debugMode, albedo.rgb, normal, normal, metallic, roughness);

    float3 lightDirection = normalize(-sceneBuffer.lightDirection.xyz);
    float nDotL = max(dot(normal, lightDirection), 0.0);

    float3 diffuse = albedo.rgb * (1.0 - metallic) * sceneBuffer.lightColor.rgb * nDotL;
    float3 ambient = albedo.rgb * sceneBuffer.ambientColor.rgb;

    return float4(diffuse + ambient, albedo.a);
}
