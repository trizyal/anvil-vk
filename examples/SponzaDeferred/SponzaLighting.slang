// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

#include "shared/PushConstants.h"

import PBR;

struct SceneData
{
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
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
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    return float4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
}

[shader("fragment")]
float4 fragmentMain(float4 pos : SV_Position) : SV_Target
{
    int2 texelCoord = int2(pos.xy);
    float4 albedo = gAlbedo.Load(int3(texelCoord, 0));
    float3 normal = gNormal.Load(int3(texelCoord, 0)).xyz;
    float4 pbr    = gPBR.Load(int3(texelCoord, 0));
    float3 wPos   = gWorldPosition.Load(int3(texelCoord, 0)).xyz;

    float roughness = pbr.g;
    float metallic  = pbr.b;

    // View, Light, and Half vectors
    float3 N = normalize(normal);
    float3 V = normalize(pushConstants.cameraPosition.xyz - wPos);
    float3 L = normalize(-sceneBuffer.lightDirection.xyz);
    float3 H = normalize(V + L);

    // Base Reflectivity (F0)
    // 0.04 is a standard base reflectivity for non-metals (dielectrics).
    // Metals tint their reflections using their albedo color.
    float3 F0 = float3(0.04);
    F0 = lerp(F0, albedo.rgb, metallic);

    // Cook-Torrance BRDF components
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    float3 F  = FresnelSchlick(max(dot(H, V), 0.0), F0);

    // Energy Conservation: Specular vs Diffuse
    float3 kS = F;
    float3 kD = float3(1.0) - kS;
    kD *= 1.0 - metallic; // Pure metals have no diffuse light

    // Specular Calculation
    float3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular     = numerator / denominator;

    // Outgoing Radiance (Lo)
    float NdotL = max(dot(N, L), 0.0);
    float3 Lo = (kD * albedo.rgb / PI + specular) * sceneBuffer.lightColor.rgb * NdotL;

    // Ambient Lighting
    float3 ambient = albedo.rgb * sceneBuffer.ambientColor.rgb;
    float3 color = ambient + Lo;

    return float4(color, albedo.a);
}