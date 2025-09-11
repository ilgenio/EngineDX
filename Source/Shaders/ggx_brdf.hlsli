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

float D_GGX(float roughness, float NdotH)
{
    float a = roughness * roughness;
    float denom = NdotH * NdotH * (a - 1.0) + 1.0;

    if(denom > 0.0)
        return a / (PI * denom * denom);

    return 0.0;
}

float V_GGX(float NdotV, float NdotL, float roughness)
{
    float a = roughness * roughness;
    float GGX_V = NdotL * sqrt(NdotV*NdotV + (1-a) +a );
    float GGX_L = NdotV * sqrt(NdotL*NdotL + (1-a) +a );
    float denom = GGX_V + GGX_L;

    if(denom > 0.0)
        return 0.5 / (PI * denom * denom);

    return 0.0;
}

float3 GGX(float3 L, float3 N, float3 V, float3 R, float3 Cs, float roughness)
{
    float3 H = normalize(V + L);

    float3 F = schlick(Cs, saturate(dot(L, H)));
    float D = D_GGX(roughness, saturate(dot(N, H)));
    float Vis = V_GGX(saturate(dot(N, V)), saturate(dot(N, L)), roughness);

    return 0.25 * F * D * Vis;
}

#endif // _GGX_BRDF_HLSLI