// TextureCube.slang

struct PushConstants
{
    float4x4 renderMatrix;
};

// [[vk::push_constant]] tells Slang to map this to Vulkan Push Constants
[[vk::push_constant]] ConstantBuffer<PushConstants> pushConstants;

struct VertexInput
{
    float3 position : POSITION;
    float3 color    : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;

    // Apply the C++ matrix transform
    // This multiplication if other way around, was breaking things
    output.position = mul(float4(input.position,1), pushConstants.renderMatrix);
    output.color = input.color;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}