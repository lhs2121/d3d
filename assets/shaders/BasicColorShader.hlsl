struct VS_INPUT
{
    float4 POS : POSITION;
};

struct VS_OUTPUT
{
    float4 POS : SV_POSITION;
};

cbuffer Transform : register(b0)
{
    float4x4 matWorld;
    float4x4 matView;
    float4x4 matProjection;
};

VS_OUTPUT BasicColorShader_VS(VS_INPUT input)
{
    VS_OUTPUT output;
    
    output.POS = mul(input.POS, matWorld);
    output.POS = mul(output.POS, matView);
    output.POS = mul(output.POS, matProjection);
    
    return output;
}

cbuffer Color : register(b1)
{
    float4 color;
}

float4 BasicColorShader_PS(VS_OUTPUT input) : SV_Target
{
    return color;
}
