Texture2D colorTex : register(t0);
SamplerState colorSamp : register(s0);

float4 exercise5PS(float2 coord : TEXCOORD) : SV_TARGET
{
    return colorTex.Sample(colorSamp, coord);
}

