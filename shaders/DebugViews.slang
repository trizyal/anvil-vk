// Copyright (C) 2026 trizyal
// SPDX-License-Identifier: GPL-3.0-only

module DebugViews;

public enum DebugMode : uint
{
    None                  = 0,
    BaseColor             = 1,
    GeometryNormal        = 2,
    RawNormalMap          = 3,
    WorldNormal           = 4,
    Metallic              = 5,
    Roughness             = 6,
    Depth                 = 7,
    OverdrawComplexity    = 8,
    OvershadingComplexity = 9
};

// Standardized engine function to format debug outputs
public float4 GetDebugColor(uint currentMode, float3 baseColor, float3 worldNormal, float3 mappedNormal, float metallic, float roughness)
{
    switch (currentMode)
    {
        case DebugMode.BaseColor:   return float4(baseColor, 1.0);
        case DebugMode.GeometryNormal: return float4(worldNormal * 0.5 + 0.5, 1.0);
        case DebugMode.WorldNormal:   return float4(mappedNormal * 0.5 + 0.5, 1.0);
        case DebugMode.Metallic:    return float4(metallic, metallic, metallic, 1.0);
        case DebugMode.Roughness:   return float4(roughness, roughness, roughness, 1.0);
        default:                    return float4(0, 0, 0, 0); // Unused
    }
}
