// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

struct PushConstants
{
    float4 cameraPosition;
    uint debugMode;
}

[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants;

[[vk::binding(0, 0)]] Sampler2D gAlbedo;
[[vk::binding(1, 0)]] Sampler2D gNormal;
[[vk::binding(2, 0)]] Sampler2D gPBR;
[[vk::binding(3, 0)]] Sampler2D gWorldPosition;

[shader("vertex")]
float4 vertexMain(uint vertexID : SV_VertexID) : SV_Position
{
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    return float4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
}

[shader("fragment")]
float4 fragmentMain(float4 pos : SV_Position) : SV_Target {
    int3 texelCoord = int3(int2(pos.xy), 0);
    float4 albedo = gAlbedo.Load(texelCoord);
    float3 normal = gNormal.Load(texelCoord).xyz;
    float4 pbr    = gPBR.Load(texelCoord);
    float3 wpos   = gWorldPosition.Load(texelCoord).xyz;

    switch (pushConstants.debugMode) {
        case 1: return float4(albedo.rgb, 1.0);
        case 4: return float4(normal * 0.5 + 0.5, 1.0);
        case 5: return float4(pbr.b, pbr.b, pbr.b, 1.0);
        case 6: return float4(pbr.g, pbr.g, pbr.g, 1.0);
        case 7: {
            float d = distance(pushConstants.cameraPosition.xyz, wpos) / 50.0f;
            return float4(d, d, d, 1.0);
        }
        default: return float4(1.0, 0.0, 1.0, 1.0);
    }
}
