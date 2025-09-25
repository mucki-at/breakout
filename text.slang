struct GlyphVertex {
    float2 offset;
    float2 texcoord;
};


struct CharacterData {
    float2 position;
};

struct VertexOutput
{
    float4 sv_position : SV_Position;
    float4 color;
    float2 texCoord;
};

struct Constants
{
    float4x4 transformation;
};
ConstantBuffer<Constants> constants;
uniform Sampler2D texture;

layout(push_constant) struct PushConstants
{
    CharacterData character;
} push;

[shader("vertex")]
VertexOutput vertMain(GlyphVertex glyph) {
    VertexOutput output;
    float2 pos = push.character.position + glyph.offset;

    output.sv_position = mul(constants.transformation, float4(pos, 0.0, 1.0));
    output.texCoord = glyph.texcoord;
    output.color = float4(1);
    return output;
}


[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target
{
    float coverage = texture.Sample(inVert.texCoord).r;
    return inVert.color * float4(coverage);
}