static float4 vertices[4] = float4[]
(
    float4(-1.0, -1.0, 0.0f, 0.0f),
    float4( 1.0, -1.0, 1.0f, 0.0f),
    float4(-1.0,  1.0, 0.0f, 1.0f),
    float4( 1.0,  1.0, 1.0f, 1.0f)
);

layout(push_constant) struct PushConstants
{
    float chaos;
    float confuse;
    float shake;
} push;

struct VertexOutput
{
    float4 sv_position : SV_Position;
    float2 texCoord;
};

[shader("vertex")]
VertexOutput vertMain(uint vId : SV_VertexID) {
    VertexOutput output;
    output.sv_position = float4(vertices[vId].xy, 0.0, 1.0);

    float2 texture = vertices[vId].zw;
    if (push.chaos > 0.0f)
    {
        float strength = 0.3;
        output.texCoord = float2(texture.x + sin(push.chaos) * strength, texture.y + cos(push.chaos) * strength);
    }
    else if (push.confuse > 0.0f)
    {
        output.texCoord = float2(1.0) - texture;
    }
    else
    {
        output.texCoord = texture;
    }
    if (push.shake > 0.0f)
    {
        float strength = 0.3f;
        output.sv_position.x += cos(push.shake * 10) * push.shake * strength;
        output.sv_position.y += cos(push.shake * 15) * push.shake * strength;
    }
    return output;
}

layout(set = 0, offset = 0) uniform Sampler2D image;

static float offset = 1.0f / 256.0f;
static float2 offsets[9] = {
    { -offset, -offset },
    { 0.0f, -offset },
    { offset, -offset },
    { -offset, 0.0f },
    { 0.0f, 0.0f },
    { offset, 0.0f },
    { -offset, offset },
    { 0.0f, offset },
    { offset, offset }
};
static float edge_kernel[9] = {
    -1.0f, -1.0f, -1.0f,
    -1.0f,  8.0f, -1.0f,
    -1.0f, -1.0f, -1.0f
};

static float blur_kernel[9] = {
    1.0f / 16.0f, 2.0f / 16.0f, 1.0f / 16.0f,
    2.0f / 16.0f, 4.0f / 16.0f, 2.0f / 16.0f,
    1.0f / 16.0f, 2.0f / 16.0f, 1.0f / 16.0f
};


[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target
{
    float4 color=float4(0.0);
    float3 sample[9];
    // sample from texture offsets if using convolution matrix
    if ((push.chaos > 0.0f) || (push.shake > 0.0f))
        for (int i = 0; i < 9; i++)
            sample[i] = image.Sample(inVert.texCoord + offsets[i]).rgb;

    // process effects
    if (push.chaos > 0.0f)
    {
        for (int i = 0; i < 9; i++)
            color.rgb += sample[i] * edge_kernel[i];
        color.a = 1.0f;
    }
    else if (push.confuse > 0.0f)
    {
        color.rgb = 1.0 - image.Sample(inVert.texCoord).rgb;
        color.a = 1.0f;
    }
    else if (push.shake > 0.0f)
    {
        for (int i = 0; i < 9; i++)
            color.rgb += sample[i] * blur_kernel[i];
        color.a = 1.0f;
    }
    else
    {
        color = image.Sample(inVert.texCoord);
    }

    return color;
}