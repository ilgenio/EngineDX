#include "Exercise6.hlsli"

Texture2D diffuseTex : register(t0);
SamplerState diffuseSamp : register(s0);

float4 exercise6PS(float3 worldPos : POSITION, float3 normal : NORMAL, float2 coord : TEXCOORD) : SV_TARGET
{
    float3 Cd       = hasDiffuseTex ? diffuseTex.Sample(diffuseSamp, coord).rgb * diffuseColour.rgb : diffuseColour.rgb;
    float3 N        = normalize(normal);
    float dotNL     = saturate(-dot(L, N));
    float3 colour   = Cd * Kd * dotNL * Lc +Ac * Cd;

    return float4(colour, 1.0);
}

