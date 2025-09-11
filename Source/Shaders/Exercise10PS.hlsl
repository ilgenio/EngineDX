#include "Exercise10.hlsli"
#include "ggx_brdf.hlsli"

Texture2D baseColourTex : register(t3);
Texture2D metallicRoughnessTex : register(t4);
SamplerState materialSamp : register(s0);

float3 computeLighting(Ambient ambient, float3 Cd)
{
    return ambient.Lc*Cd;
}

float3 computeLighting(float3 V, float3 N, Directional light, float3 Cd, float3 Cs, float roughness)
{
    float3 L    = normalize(-light.Ld);
    float3 R    = reflect(-L, N);
    
    float NdotL = saturate(dot(L, N));

    return (Lambert(Cd) + GGX(L, N, V, R, Cs, roughness)) * NdotL * light.Lc * light.intensity ;
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
    float3 Ldiff = light.Lp - worldPos;
    float3 L     = normalize(Ldiff);
    float3 R     = reflect(-L, N);

    float sqDist = dot(Ldiff, Ldiff);

    float attenuation = pointFalloff(sqDist, light.sqRadius);
    
    return (Lambert(Cd) + GGX(L, N, V, R, Cs, roughness)) * light.Lc * light.intensity * saturate(dot(L, N)) * attenuation;
}

float3 computeLighting(float3 V, float3 N, Spot light, float3 worldPos, float3 Cd, float3 Cs, float roughness)
{
    float3 Ldiff = light.Lp - worldPos;
    float3 L     = normalize(Ldiff);
    float3 R     = reflect(-L, N);

    float dist  = dot(-Ldiff, light.Ld);
    float attenuation = pointFalloff(dist*dist, light.sqRadius);

    float cosDist = dot(-L, light.Ld);
    attenuation *= spotFalloff(cosDist, light.inner, light.outter);
    
    return ( Lambert(Cd)+ GGX(L, N, V, R, Cs, roughness) ) * light.Lc * light.intensity * saturate(dot(L, N)) * attenuation;
}

void getMaterialProperties(out float3 Cd, out float3 Cs, out float roughness, in float2 coord)
{
    float3 baseColour = material.hasBaseColourTex ?  baseColourTex.Sample(materialSamp, coord).rgb * material.baseColour.rgb : material.baseColour.rgb;
    float2 metallicRoughness = material.hasMetallicRoughnessTex ? metallicRoughnessTex.Sample(materialSamp, coord).bg * float2(material.metallicFactor, material.roughnessFactor) : float2(material.metallicFactor, material.roughnessFactor);

    Cd = baseColour * (1.0 - metallicRoughness.x);
    Cs = lerp(0.04, baseColour, metallicRoughness.x);
    roughness = metallicRoughness.y*metallicRoughness.y; // Perceptural roughness
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

