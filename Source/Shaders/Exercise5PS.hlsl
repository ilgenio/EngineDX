
cbuffer Material : register(b1)
{
    float4 colour;
    bool hasColourTex;
};

Texture2D colourTex : register(t0);
SamplerState colourSamp : register(s0);

float4 exercise5PS(float2 coord : TEXCOORD) : SV_TARGET
{
    return hasColourTex ? colourTex.Sample(colourSamp, coord)*colour : colour;
}

