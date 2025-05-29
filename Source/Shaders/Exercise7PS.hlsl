#include "Exercise7.hlsli"

Texture2D diffuseTex : register(t0);
SamplerState diffuseSamp : register(s0);

float3 shlick(float3 rf0, float dotNL)
{
    return rf0 + (1 - rf0) * pow(dotNL, 5);
}

float4 exercise7PS(float3 worldPos : POSITION, float3 normal : NORMAL, float2 coord : TEXCOORD) : SV_TARGET
{
    float3 Cd       = hasDiffuseTex ? diffuseTex.Sample(diffuseSamp, coord).rgb * diffuseColour.rgb : diffuseColour.rgb;
    float3 N        = normalize(normal);
    float3 R        = reflect(L, N);
    float3 V        = normalize(viewPos - worldPos);
    float3 dotVR    = saturate(dot(V, R));
    float dotNL     = saturate(-dot(L, N));
    
    float rf0Max   = max(max(specularColour.r, specularColour.g), specularColour.b);
    float3 fresnel = shlick(specularColour, dotNL);
    float3 colour  = ((Cd * (1.0 - rf0Max)) / PI + ((shininess + 2.0) / (2 * PI)) * fresnel * pow(dotVR, shininess)) * Lc * dotNL;

    return float4(colour, 1.0);
}

