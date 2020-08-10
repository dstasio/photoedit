struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

#if VERTEX_HLSL // ===================================================

struct VS_INPUT
{
    float2 pos    : POSITION;
    float3 col    : COLOR;
};

cbuffer Matrices: register(b0)
{
    float4x4 model;
}

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = mul(model, float4(input.pos.xy, 0.f, 1.f));
    output.col = input.col;

    return output;
}

#elif PIXEL_HLSL // ==================================================

#define MOUSE_INACTIVE 0
#define MOUSE_HOT      1
#define MOUSE_ACTIVE   2
#define MOUSE_STUB     3

cbuffer flags
{
    uint mouse;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
    float4 color = float4(0.4f, 0.8f, 0.5f, 1.f);
    if (mouse == MOUSE_HOT) {
        color = float4(0.3f, 0.8f, 0.5f, 1.f);
    }
    else if (mouse == MOUSE_ACTIVE) {
        color = float4(0.2f, 0.6f, 0.3f, 1.f);
    }
    else if (mouse == MOUSE_STUB) {
        color = float4(0.3f, 0.3f, 0.3f, 1.f);
    }
    //color = float4(input.col, 1.f);
    //float4 output = color*font.Sample(texture_sampler_state, input.txc);
    return color;
}

#endif
