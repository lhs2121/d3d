struct VS_INPUT
{
    float4 POS : POSITION;
    float2 TEX : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 POS : SV_POSITION;
    float2 TEX : TEXCOORD;
};

cbuffer Transform : register(b0)
{
    float4x4 matWorld;
    float4x4 matView;
    float4x4 matProjection;
};


VS_OUTPUT BasicSprite2DShader_VS(VS_INPUT input)
{
    VS_OUTPUT output;
    
    output.POS = mul(input.POS, matWorld);
    output.POS = mul(output.POS, matView);
    output.POS = mul(output.POS, matProjection);
    output.TEX = input.TEX;
    return output;
}

cbuffer SpriteData : register(b0)
{
    float2 ResizeRatio;
    float2 Offset;
}

Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

float4 BasicSprite2DShader_PS(VS_OUTPUT input) : SV_Target
{
    float2 TexLocation = input.TEX * ResizeRatio;
    TexLocation.x += Offset.x;
    TexLocation.y += Offset.y;
    
    float4 TexColor = Texture.Sample(Sampler, TexLocation);
    
    return TexColor;
}
