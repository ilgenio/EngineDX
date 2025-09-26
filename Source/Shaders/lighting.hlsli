#ifndef _LIGHTING_HLSLI_
#define _LIGHTING_HLSLI_

#include "lights.hlsli"

float3 computeLighting(float3 V, float3 N, float3 L, float3 lightColour, float3 baseColour, float roughness, float metallic)
{
    float3 H = normalize(V + L);
    
    float NdotL = saturate(dot(L, N));
    float VdotH = saturate(dot(V, H));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));

    float3 metalF = F_Schlick(baseColour, 1.0, VdotH);
    float dielectricF = F_Schlick(0.04, 1.0, VdotH);

    float3 diffuse = Lambert(baseColour) * NdotL * lightColour;
    float3 specular = GGX(NdotL, NdotV, NdotH, roughness) * NdotL * lightColour;

    float3 dielectric = diffuse + specular*dielectricF;
    float3 metal = specular * metalF;

    return lerp(dielectric, metal, metallic);
}

float3 computeLighting(float3 V, float3 N, Directional light, float3 baseColour, float roughness, float metallic)
{
    float3 L = normalize(-light.Ld);
    return computeLighting(V, N, L, light.Lc * light.intensity, baseColour, roughness, metallic);
}

float3 computeLighting(float3 V, float3 N, Point light, float3 worldPos, float3 baseColour, float roughness , float metallic)
{
    float3 Ldiff = light.Lp - worldPos;
    float3 L     = normalize(Ldiff);

    float sqDist = dot(Ldiff, Ldiff);

    float attenuation = pointFalloff(sqDist, light.sqRadius);
    
    return computeLighting(V, N, L, light.Lc * light.intensity, baseColour, roughness, metallic) * attenuation;
}

float3 computeLighting(float3 V, float3 N, Spot light, float3 worldPos, float3 baseColour, float roughness, float metallic)
{
    float3 Ldiff = light.Lp - worldPos;
    float3 L     = normalize(Ldiff);

    float dist  = dot(-Ldiff, light.Ld);
    float attenuation = pointFalloff(dist*dist, light.sqRadius);

    float cosDist = dot(-L, light.Ld);
    attenuation *= spotFalloff(cosDist, light.inner, light.outter);
    
    return computeLighting(V, N, L, light.Lc * light.intensity, baseColour, roughness, metallic)*attenuation;
}


#endif // _LIGHTING_HLSLI_