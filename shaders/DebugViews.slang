module DebugViews;

public enum DebugMode : uint
{
        None        = 0,
        BaseColor   = 1,
        WorldNormal = 2,
        NormalMap   = 3,
        Metallic    = 4,
        Roughness   = 5
};

// Standardized engine function to format debug outputs
public float4 GetDebugColor(uint currentMode, float3 baseColor, float3 worldNormal, float3 mappedNormal, float metallic, float roughness)
{
    switch (currentMode)
    {
        case DebugMode.BaseColor:   return float4(baseColor, 1.0);
        case DebugMode.WorldNormal: return float4(worldNormal * 0.5 + 0.5, 1.0);
        case DebugMode.NormalMap:   return float4(mappedNormal * 0.5 + 0.5, 1.0);
        case DebugMode.Metallic:    return float4(metallic, metallic, metallic, 1.0);
        case DebugMode.Roughness:   return float4(roughness, roughness, roughness, 1.0);
        default:                    return float4(0, 0, 0, 0); // Unused
    }
}
