static float4 vertices[4] = float4[]
(
    float4(-0.5, -0.5, 0.0f, 0.0f),
    float4( 0.5, -0.5, 1.0f, 0.0f),
    float4(-0.5,  0.5, 0.0f, 1.0f),
    float4( 0.5,  0.5, 1.0f, 1.0f)
);

struct QuadData {
    float4 color;
    float2x2 transform;
    float2 position;
};

struct VertexOutput
{
    float4 sv_position : SV_Position;
    float4 color;
    float2 texCoord;
};

layout(push_constant) struct PushConstants
{
    float4x4 transform;
    QuadData quad;
} push;

[shader("vertex")]
VertexOutput vertMain(uint vId : SV_VertexID) {
    VertexOutput output;
    float2 pos = mul(push.quad.transform, vertices[vId].xy)+push.quad.position;
    
    output.sv_position = mul(push.transform, float4(pos, 0.0, 1.0));
    output.texCoord = vertices[vId].zw;
    output.color = push.quad.color;
    return output;
}

layout(set = 0, offset = 0) uniform Sampler2D texture;

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target
{
    return inVert.color * texture.Sample(inVert.texCoord);
}