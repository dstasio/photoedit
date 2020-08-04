struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 txc : TEXCOORD;
};

#if VERTEX_HLSL // ===================================================

struct VS_INPUT
{
    float2 pos    : POSITION;
    float2 txc    : TEXCOORD;
};

cbuffer Matrices: register(b0)
{
    float4x4 model;
}

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = mul(model, float4(input.pos.xy, 0.f, 1.f));
    output.txc = input.txc;

    return output;
}

#elif PIXEL_HLSL // ==================================================

#define MOUSE_INACTIVE 0
#define MOUSE_HOT      1
#define MOUSE_ACTIVE   2

cbuffer flags
{
    uint mouse;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float4 color = float4(0.8f, 0.8f, 0.f, 1.f);
    if (mouse == MOUSE_HOT) {
        color = float4(0.3f, 0.8f, 0.5f, 1.f);
    }
    else if (mouse == MOUSE_ACTIVE) {
        color = float4(0.2f, 0.6f, 0.3f, 1.f);
    }
    //float4 output = color*font.Sample(texture_sampler_state, input.txc);
    return color;
}

#endif
