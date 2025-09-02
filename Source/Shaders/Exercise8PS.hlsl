#include "Exercise8.hlsli"

Texture2D diffuseTex : register(t3);
SamplerState diffuseSamp : register(s0);

float3 schlick(float3 rf0, float dotNL)
{
    return rf0 + (1 - rf0) * pow(1.0-dotNL, 5);
}

float3 phongBRDF(float3 L, float3 N, float3 V, float3 R, float3 Cd, float3 Cs, float shininess, float3 Lc)
{
    float3 dotVR   = saturate(dot(V, R));
    float dotNL    = saturate(-dot(L, N));
    float rf0Max   = max(max(Cs.r, Cs.g), Cs.b);

    float3 fresnel = schlick(Cs, dotNL);
    float3 colour = (Cd * (1.0 - rf0Max) + ((shininess + 2.0) / (2)) * fresnel * pow(dotVR, shininess)) * Lc * dotNL;

    return colour;
}

float3 computeLighting(Ambient ambient, float3 Cd)
{
    return ambient.Lc*Cd;
}

float3 computeLighting(float3 V, float3 N, Directional light, float3 Cd, float3 Cs, float shininess)
{
    float3 L = normalize(light.Ld);
    float3 R = reflect(L, N);

    return phongBRDF(L, N, V, R, Cd, Cs, shininess, light.Lc * light.intensity);
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

float3 computeLighting(float3 V, float3 N, Point light, float3 worldPos, float3 Cd, float3 Cs, float shininess)
{
    float3 Ldiff = worldPos - light.Lp;
    float3 L     = normalize(Ldiff);
    float3 R     = reflect(L, N);

    float sqDist = dot(Ldiff, Ldiff);

    float attenuation = pointFalloff(sqDist, light.sqRadius);

    return phongBRDF(L, N, V, R, Cd, Cs, shininess, light.Lc * light.intensity) * attenuation;
}

float3 computeLighting(float3 V, float3 N, Spot light, float3 worldPos, float3 Cd, float3 Cs, float shininess)
{
    float3 Ldiff = worldPos - light.Lp;
    float3 L     = normalize(Ldiff);
    float3 R     = reflect(L, N);

    float sqDist  = dot(Ldiff, light.Ld);
    float attenuation = pointFalloff(sqDist, light.sqRadius);

    float cosDist = dot(L, light.Ld);
    attenuation *= spotFalloff(cosDist, light.inner, light.outter);

    return phongBRDF(L, N, V, R, Cd, Cs, shininess, light.Lc * light.intensity) * attenuation;
}

float4 exercise8PS(float3 worldPos : POSITION, float3 normal : NORMAL, float2 coord : TEXCOORD) : SV_TARGET
{
    float3 Cd = material.hasDiffuseTex ? diffuseTex.Sample(diffuseSamp, coord).rgb * material.diffuseColour.rgb : material.diffuseColour.rgb;
    float3 Cs = material.specularColour.rgb;
    
    float3 V  = normalize(viewPos - worldPos);
    float3 N  = normalize(normal);
    
    float3 colour = computeLighting(ambient, Cd);

    for (uint i = 0; i < numDirLights; i++)
        colour += computeLighting(V, N, dirLights[i], Cd, Cs, material.shininess);
    
    for (uint i = 0; i < numPointLights; i++) 
        colour += computeLighting(V, N, pointLights[i], worldPos, Cd, Cs, material.shininess);

    for( uint i = 0; i< numSpotLights; i++)
        colour += computeLighting(V, N, spotLights[i], worldPos, Cd, Cs, material.shininess);

    // Reinhard tone mapping (HDR to LDR)
    float3 ldr = colour.rgb / (colour.rgb + 1.0);
    
    return float4(ldr, 1.0);
}

