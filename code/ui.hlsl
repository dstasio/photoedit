#define MOUSE_INACTIVE 0
#define MOUSE_HOT      1
#define MOUSE_ACTIVE   2
#define MOUSE_STUB     3
#define DEPTH_0 0
#define DEPTH_1 1
#define DEPTH_2 2
#define DEPTH_3 3
#define DEPTH_4 4

cbuffer flags
{
    uint mouse;
    uint depth;
}

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
    float4x4 ortho;
}

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = mul(ortho, mul(model, float4(input.pos.xy, (float)depth/10.f, 1.f)));
    output.col = input.col;

    return output;
}

#elif PIXEL_HLSL // ==================================================

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
    //color = float4(0.8*input.col, 1.f);
    //float4 output = color*font.Sample(texture_sampler_state, input.txc);
    return color;
}

#endif
