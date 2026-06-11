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

struct GRID_VS_INPUT
{
    float4 POS : POSITION;
    float2 TEX : TEXCOORD;
    float4 TILE0 : TILE0;
    float4 TILE1 : TILE1;
};

struct BATCH_VS_INPUT
{
    float4 POS : POSITION;
    float2 TEX : TEXCOORD;
    float4 SPRITE0 : SPRITE0;
    float4 SPRITE1 : SPRITE1;
    float4 SPRITE2 : SPRITE2;
    float4 COLOR : COLOR0;
};

struct BATCH_VS_OUTPUT
{
    float4 POS : SV_POSITION;
    float2 TEX : TEXCOORD;
    float4 COLOR : COLOR0;
};

struct GLYPH_VS_INPUT
{
    float4 POS : POSITION;
    float2 TEX : TEXCOORD;
    float4 GLYPH0 : GLYPH0;
    float4 ROWS0 : ROWS0;
    float4 ROWS1 : ROWS1;
    float4 COLOR : COLOR0;
};

struct GLYPH_VS_OUTPUT
{
    float4 POS : SV_POSITION;
    float2 TEX : TEXCOORD;
    float4 ROWS0 : ROWS0;
    float4 ROWS1 : ROWS1;
    float4 COLOR : COLOR0;
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

VS_OUTPUT BasicSprite2DShader_GridVS(GRID_VS_INPUT input)
{
    VS_OUTPUT output;

    output.POS = float4(
        input.POS.x * input.TILE0.z + input.TILE0.x,
        input.POS.y * input.TILE0.z + input.TILE0.y,
        input.TILE0.w,
        1.0f);
    output.POS = mul(output.POS, matWorld);
    output.POS = mul(output.POS, matView);
    output.POS = mul(output.POS, matProjection);
    output.TEX = input.TEX * input.TILE1.zw + input.TILE1.xy;
    return output;
}

BATCH_VS_OUTPUT BasicSprite2DShader_BatchVS(BATCH_VS_INPUT input)
{
    BATCH_VS_OUTPUT output;

    float2 scaled = float2(input.POS.x * input.SPRITE0.z, input.POS.y * input.SPRITE0.w);
    float sinRotation = input.SPRITE1.x;
    float cosRotation = input.SPRITE1.y;
    float2 rotated = float2(
        scaled.x * cosRotation - scaled.y * sinRotation,
        scaled.x * sinRotation + scaled.y * cosRotation);

    output.POS = float4(input.SPRITE0.xy + rotated, input.SPRITE1.z, 1.0f);
    output.POS = mul(output.POS, matView);
    output.POS = mul(output.POS, matProjection);
    output.TEX = input.TEX * input.SPRITE2.zw + input.SPRITE2.xy;
    output.COLOR = input.COLOR;
    return output;
}

GLYPH_VS_OUTPUT BasicSprite2DShader_GlyphVS(GLYPH_VS_INPUT input)
{
    GLYPH_VS_OUTPUT output;

    output.POS = float4(
        input.POS.x * input.GLYPH0.z + input.GLYPH0.x,
        input.POS.y * input.GLYPH0.w + input.GLYPH0.y,
        input.ROWS1.w,
        1.0f);
    output.POS = mul(output.POS, matView);
    output.POS = mul(output.POS, matProjection);
    output.TEX = input.TEX;
    output.ROWS0 = input.ROWS0;
    output.ROWS1 = input.ROWS1;
    output.COLOR = input.COLOR;
    return output;
}

cbuffer SpriteData : register(b0)
{
    float2 ResizeRatio;
    float2 Offset;
    float4 Color;
}

Texture2D Texture : register(t0);
SamplerState Sampler : register(s0);

float4 BasicSprite2DShader_PS(VS_OUTPUT input) : SV_Target
{
    float2 TexLocation = input.TEX * ResizeRatio;
    TexLocation.x += Offset.x;
    TexLocation.y += Offset.y;
    
    float4 TexColor = Texture.Sample(Sampler, TexLocation);
    
    return TexColor * Color;
}

float4 BasicSprite2DShader_BatchPS(BATCH_VS_OUTPUT input) : SV_Target
{
    return Texture.Sample(Sampler, input.TEX) * input.COLOR;
}

float4 BasicSprite2DShader_GlyphPS(GLYPH_VS_OUTPUT input) : SV_Target
{
    float column = min(4.0f, floor(saturate(input.TEX.x) * 5.0f));
    float row = min(6.0f, floor(saturate(input.TEX.y) * 7.0f));
    float bits = input.ROWS0.x;

    if (row < 0.5f)
        bits = input.ROWS0.x;
    else if (row < 1.5f)
        bits = input.ROWS0.y;
    else if (row < 2.5f)
        bits = input.ROWS0.z;
    else if (row < 3.5f)
        bits = input.ROWS0.w;
    else if (row < 4.5f)
        bits = input.ROWS1.x;
    else if (row < 5.5f)
        bits = input.ROWS1.y;
    else
        bits = input.ROWS1.z;

    float bitMask = exp2(4.0f - column);
    float filled = fmod(floor(bits / bitMask), 2.0f);
    clip(filled - 0.5f);
    return input.COLOR;
}
