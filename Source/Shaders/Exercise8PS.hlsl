#include "Exercise8.hlsli"

Texture2D diffuseTex : register(t0);
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

float epicFalloff(float sqDist, float sqRadius)
{
    float falloff = (1.0-saturate(sqDist*sqDist/sqRadius*sqRadius))/(sqDist+1);   

    return falloff*falloff;
}

float3 computeLighting(float3 V, float3 N, Point light, float3 worldPos, float3 Cd, float3 Cs, float shininess)
{
    float3 Ldiff = worldPos - light.Lp;
    float3 L     = normalize(Ldiff);
    float3 R     = reflect(L, N);

    float sqDist = dot(Ldiff, Ldiff);

    float attenuation = epicFalloff(sqDist, light.sqRadius);

    return phongBRDF(L, N, V, R, Cd, Cs, shininess, light.Lc * light.intensity) * attenuation;
}

float3 computeLighting(float3 V, float3 N, Spot light, float3 worldPos, float3 Cd, float3 Cs, float shininess)
{
    float3 Ldiff = worldPos - light.Lp;
    float3 L     = normalize(Ldiff);
    float3 R     = reflect(L, N);

    float dist = dot(Ldiff, light.Ld);
    float distAtt = epicFalloff(dist * dist, light.sqRadius);

    float C = dot(L, light.Ld);
    float angleAtt = saturate((C - light.outter) / (light.inner - light.outter));

    return phongBRDF(L, N, V, R, Cd, Cs, shininess, light.Lc * light.intensity) * distAtt * angleAtt;
}

float4 exercise8PS(float3 worldPos : POSITION, float3 normal : NORMAL, float2 coord : TEXCOORD) : SV_TARGET
{
    float3 Cd = material.hasDiffuseTex ? diffuseTex.Sample(diffuseSamp, coord).rgb * material.diffuseColour.rgb : material.diffuseColour.rgb;
    float3 V  = normalize(viewPos - worldPos);
    float3 N  = normalize(normal);
    
    float3 colour = computeLighting(ambient, Cd);
    colour += computeLighting(V, N, dirLight, Cd, material.specularColour, material.shininess);
    //colour += computeLighting(V, N, pointLight, worldPos, Cd, material.specularColour, material.shininess);
    //colour += computeLighting(V, N, spotLight, worldPos, Cd, material.specularColour, material.shininess);
    
    
    
    
    return float4(colour, 1.0); 
}

