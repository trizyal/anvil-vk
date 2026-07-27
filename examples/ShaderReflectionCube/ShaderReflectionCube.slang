// ShaderReflectionCube.slang

struct PushConstants
{
    float4x4 renderMatrix;
};

// [[vk::push_constant]] tells Slang to map this to Vulkan Push Constants
[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants;

// [[vk::binding(0, 0)]] forces it to use Set 0, Binding 0 to match your C++ setup.
[[vk::binding(0, 0)]]
Sampler2D texSampler;

struct VertexInput
{
    float3 position : POSITION;
    float3 color    : COLOR;
    float2 uv       : TEXCOORD;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color;
    float2 uv;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;

    // Apply the C++ matrix transform
    // This multiplication if other way around, was breaking things
    output.position = mul(float4(input.position,1), pushConstants.renderMatrix);
    output.color = input.color;
    output.uv = input.uv;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    // Sample the texture at the given UV coordinates
    float4 textureColor = texSampler.Sample(input.uv);

    return textureColor;
}