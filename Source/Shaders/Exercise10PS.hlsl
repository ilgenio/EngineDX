#include "Exercise10.hlsli"

Texture2D baseColourTex : register(t3);
Texture2D metallicRoughnessTex : register(t4);
SamplerState materialSamp : register(s0);

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
        return a / (PI * denom * denom);

    return 0.0;
}

float3 GGX(float3 L, float3 N, float3 V, float3 R, float3 Cs, float roughness)
{
    float3 H = normalize(V + L);

    float3 F = schlick(Cs, saturate(dot(L, H)));
    float D = D_GGX(roughness, saturate(dot(N, H)));
    float Vis = V_GGX(saturate(dot(N, V)), saturate(dot(N, L)), roughness);

    return F * D * Vis ;
}

float3 Lambert(float3 Cd)
{
    return Cd / PI;
}

float3 computeLighting(Ambient ambient, float3 Cd)
{
    return ambient.Lc*Cd;
}

float3 computeLighting(float3 V, float3 N, Directional light, float3 Cd, float3 Cs, float roughness)
{
    float3 L    = normalize(-light.Ld);
    float3 R    = reflect(-L, N);

    return ( Lambert(Cd) + GGX(L, N, V, R, Cs, roughness) ) * light.Lc * light.intensity * saturate(dot(L, N));
}

float pointFalloff(float sqDist, float sqRadius)
{
    float num = max(1.0-(sqDist*sqDist)/(sqRadius*sqRadius), 0.0);
    float falloff = (num*num)/(sqDist+1);   

    return falloff;
}

float spotFalloff(float cosDist, float inner, float outer)
{
    float angleAtt = saturate((cosDist - outer) / (inner - outer));

    return angleAtt*angleAtt;
}

float3 computeLighting(float3 V, float3 N, Point light, float3 worldPos, float3 Cd, float3 Cs, float roughness)
{
    float3 Ldiff = worldPos - light.Lp;
    float3 L     = normalize(Ldiff);
    float3 R     = reflect(L, N);

    float sqDist = dot(Ldiff, Ldiff);

    float attenuation = pointFalloff(sqDist, light.sqRadius);

    return (Lambert(Cd)+GGX(L, N, V, R, Cs, roughness)) * light.Lc * light.intensity*dot(N, L)*attenuation;
}

float3 computeLighting(float3 V, float3 N, Spot light, float3 worldPos, float3 Cd, float3 Cs, float roughness)
{
    float3 Ldiff = worldPos - light.Lp;
    float3 L     = normalize(Ldiff);
    float3 R     = reflect(L, N);

    float sqDist  = dot(Ldiff, light.Ld);
    float attenuation = pointFalloff(sqDist, light.sqRadius);

    float cosDist = dot(L, light.Ld);
    attenuation *= spotFalloff(cosDist, light.inner, light.outter);

    return (Lambert(Cd)+GGX(L, N, V, R, Cs, roughness))*light.Lc * light.intensity*dot(N, L)*attenuation;
}

void getMaterialProperties(out float3 Cd, out float3 Cs, out float roughness, in float2 coord)
{
    float3 baseColour = material.hasBaseColourTex ?  baseColourTex.Sample(materialSamp, coord).rgb * material.baseColour.rgb : material.baseColour.rgb;
    float2 metallicRoughness = material.hasMetallicRoughnessTex ? metallicRoughnessTex.Sample(materialSamp, coord).bg * float2(material.metallicFactor, material.roughnessFactor) : float2(material.metallicFactor, material.roughnessFactor);

    Cd = baseColour * (1.0 - metallicRoughness.x);
    Cs = lerp(0.04, baseColour, metallicRoughness.x);
    roughness = metallicRoughness.y; // Perceptural roughness
}

float4 exercise10PS(float3 worldPos : POSITION, float3 normal : NORMAL, float2 coord : TEXCOORD) : SV_TARGET
{
    float3 Cd, Cs;
    float roughness;
    getMaterialProperties(Cd, Cs, roughness, coord);

    float3 V  = normalize(viewPos - worldPos);
    float3 N  = normalize(normal);
    
    float3 colour = computeLighting(ambient, Cd);

    for (uint i = 0; i < numDirLights; i++)
        colour += computeLighting(V, N, dirLights[i], Cd, Cs, roughness);
    
    for (uint i = 0; i < numPointLights; i++) 
        colour += computeLighting(V, N, pointLights[i], worldPos, Cd, Cs, roughness);

    for( uint i = 0; i< numSpotLights; i++)
        colour += computeLighting(V, N, spotLights[i], worldPos, Cd, Cs, roughness);

    // Reinhard tone mapping (HDR to LDR)
    float3 ldr = colour.rgb / (colour.rgb + 1.0);
    
    return float4(ldr, 1.0);
}

