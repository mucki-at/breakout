static float4 vertices[4] = float4[]
(
    float4(-1.0, -1.0, 0.0f, 0.0f),
    float4( 1.0, -1.0, 1.0f, 0.0f),
    float4(-1.0,  1.0, 0.0f, 1.0f),
    float4( 1.0,  1.0, 1.0f, 1.0f)
);

struct VertexOutput
{
    float4 sv_position : SV_Position;
    float2 texCoord;
};

[shader("vertex")]
VertexOutput vertMain(uint vId : SV_VertexID) {
    VertexOutput output;
    output.sv_position = float4(vertices[vId].xy, 0.0, 1.0);
    output.texCoord = vertices[vId].zw;
    return output;
}

layout(set = 0, offset = 0) uniform Sampler2D image;

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target
{
    return image.Sample(inVert.texCoord);
}