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

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
#if 0
    float4 albedo = baseColorTexture.Sample(input.uv);

    // Alpha test for Sponza's foliage/flags
    if (albedo.a < 0.5)
    {
        discard;
    }

    float3 normal = normalize(input.worldNormal);
    float3 lightDir = normalize(-sceneBuffer.lightDirection.xyz);

    float nDotL = max(dot(normal, lightDir), 0.0);

    float3 diffuse = albedo.rgb * sceneBuffer.lightColor.rgb * nDotL;
    float3 ambient = albedo.rgb * sceneBuffer.ambientColor.rgb;

    return float4(diffuse + ambient, albedo.a);
#else
    float4 albedo = baseColorTexture.Sample(input.uv);
    if (albedo.a < 0.5) discard; // Alpha test

    // glTF PBR standard: B channel = Metallic, G channel = Roughness
    float4 metRough = metallicRoughnessTexture.Sample(input.uv);
    float roughness = metRough.g;
    float metallic = metRough.b;

    // Build the Tangent-Bitangent-Normal (TBN) Matrix
    float3 N = normalize(input.worldNormal);
    float3 T = normalize(input.worldTangent.xyz);
    T = normalize(T - dot(T, N) * N); // Re-orthogonalize T with respect to N
    float3 B = cross(N, T) * input.worldTangent.w;
    float3x3 TBN = float3x3(T, B, N);

    // Apply the Normal Map
    float3 sampledNormal = normalTexture.Sample(input.uv).xyz * 2.0 - 1.0;
    float3 normal = normalize(mul(sampledNormal, TBN));

    // Calculate basic lighting vectors
    float3 lightDir = normalize(-sceneBuffer.lightDirection.xyz);
    float3 viewDir = normalize(pushConstants.cameraPosition.xyz - input.worldPosition);
    float nDotL = max(dot(normal, lightDir), 0.0);

    // Simple PBR approximation (Diffuse)
    float3 diffuse = albedo.rgb * (1.0 - metallic) * sceneBuffer.lightColor.rgb * nDotL;
    float3 ambient = albedo.rgb * sceneBuffer.ambientColor.rgb;

    return float4(diffuse + ambient, albedo.a);
#endif
}
