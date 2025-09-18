#include "Exercise10.hlsli"
#include "ggx_brdf.hlsli"
#include "tonemap.hlsli"

Texture2D baseColourTex : register(t3);
Texture2D metallicRoughnessTex : register(t4);
SamplerState materialSamp : register(s0);

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

    float3 dielectric = lerp(diffuse, specular, dielectricF);
    float3 metal = specular * metalF;

    return lerp(dielectric, metal, metallic);
}

float3 computeLighting(float3 V, float3 N, Directional light, float3 baseColour, float roughness, float metallic)
{
    float3 L = normalize(-light.Ld);
    return computeLighting(V, N, L, light.Lc * light.intensity, baseColour, roughness, metallic);
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

void getMaterialProperties(out float3 baseColour, out float roughness, out float metallic, in float2 coord)
{
    baseColour = material.hasBaseColourTex ?  baseColourTex.Sample(materialSamp, coord).rgb * material.baseColour.rgb : material.baseColour.rgb;
    float2 metallicRoughness = material.hasMetallicRoughnessTex ? metallicRoughnessTex.Sample(materialSamp, coord).bg * float2(material.metallicFactor, material.roughnessFactor) : float2(material.metallicFactor, material.roughnessFactor);

    metallic = metallicRoughness.x;
    roughness = metallicRoughness.y * metallicRoughness.y; // Perceptural roughness
}

float4 exercise10PS(float3 worldPos : POSITION, float3 normal : NORMAL, float2 coord : TEXCOORD) : SV_TARGET
{
    float3 baseColour;
    float roughness;
    float metallic;
    getMaterialProperties(baseColour, roughness, metallic, coord);

    float3 V  = normalize(viewPos - worldPos);
    float3 N  = normalize(normal);
    
    float3 colour = 0.0; 

    for (uint i = 0; i < numDirLights; i++)
        colour += computeLighting(V, N, dirLights[i], baseColour, roughness, metallic);
    
    for (uint i = 0; i < numPointLights; i++) 
        colour += computeLighting(V, N, pointLights[i], worldPos, baseColour, roughness, metallic);

    for( uint i = 0; i< numSpotLights; i++)
        colour += computeLighting(V, N, spotLights[i], worldPos, baseColour, roughness, metallic);
    
    float3 ldr = PBRNeutralToneMapping(colour);
    
    return float4(linearTosRGB(ldr), 1.0);
}

