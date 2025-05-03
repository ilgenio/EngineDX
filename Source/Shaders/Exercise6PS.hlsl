#include "Exercise6.hlsli"

Texture2D diffuseTex : register(t0);
SamplerState diffuseSamp : register(s0);

float4 exercise6PS(float3 worldPos : POSITION, float3 normal : NORMAL, float2 coord : TEXCOORD) : SV_TARGET
{
    float3 Cd       = hasDiffuseTex ? diffuseTex.Sample(diffuseSamp, coord).rgb * diffuseColour.rgb : diffuseColour.rgb;
    float3 N        = normalize(normal);
    float3 R        = reflect(L, N);
    float3 V        = normalize(viewPos - worldPos);
    float3 dotVR    = saturate(dot(V, R));
    float dotNL     = saturate(-dot(L, N));
    
    float3 colour   = Cd * Kd * dotNL * Lc + Ac * Cd + Ks * Lc * pow(dotVR, shininess);

    return float4(colour, 1.0);
}

