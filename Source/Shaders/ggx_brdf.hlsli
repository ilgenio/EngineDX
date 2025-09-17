#ifndef _GGX_BRDF_HLSLI
#define _GGX_BRDF_HLSLI

#include "common.hlsli"

float3 Lambert(float3 Cd)
{
    return Cd / PI;
}

float3 F_Schlick(float3 rf0, float3 rf90, float dotVH)
{
    return rf0 + (rf90 - rf0) * pow(1.0-dotVH, 5);
}

float F_Schlick(float rf0, float rf90, float dotVH)
{
    return rf0 + (rf90 - rf0) * pow(1.0-dotVH, 5);
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

float GGX(float NdotL, float NdotV, float NdotH, float alphaRoughness)
{
    float D = D_GGX(alphaRoughness, NdotH);
    float Vis = V_GGX(NdotV, NdotL, alphaRoughness);

    return 0.25 * D * Vis;
}

#endif // _GGX_BRDF_HLSLI