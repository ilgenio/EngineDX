#ifndef _GGX_BRDF_HLSLI
#define _GGX_BRDF_HLSLI

#include "common.hlsli"

float3 Lambert(float3 Cd)
{
    return Cd / PI;
}

float3 schlick(float3 rf0, float dotLH)
{
    return rf0 + (1 - rf0) * pow(1.0-dotLH, 5);
}

float D_GGX(float alphaRoughness, float NdotH)
{
    float a2 = alphaRoughness * alphaRoughness;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;

    if(denom > 0.0)
        return a2 / (PI * denom * denom);

    return 0.0;
}

float V_GGX(float NdotV, float NdotL, float alphaRoughness)
{
    float a2 = alphaRoughness * alphaRoughness;

    float GGX_V = NdotL * sqrt(NdotV*NdotV * (1.0-a2) + a2 );
    float GGX_L = NdotV * sqrt(NdotL*NdotL * (1.0-a2) + a2 );
    float denom = GGX_V + GGX_L;

    if(denom > 0.0)
        return 0.5 / (denom);

    return 0.0;
}

float3 GGX(float3 L, float3 N, float3 V, float3 R, float3 Cs, float alphaRoughness)
{
    float3 H = normalize(V + L);

    float3 F = schlick(Cs, saturate(dot(L, H)));
    float D = D_GGX(alphaRoughness, saturate(dot(N, H)));
    float Vis = V_GGX(saturate(dot(N, V)), saturate(dot(N, L)), alphaRoughness);

    return 0.25 * F * D * Vis;
}

#endif // _GGX_BRDF_HLSLI