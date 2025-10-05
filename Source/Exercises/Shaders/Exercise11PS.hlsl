#include "Exercise11.hlsli"
#include "ibl.hlsli"
#include "tonemap.hlsli"

TextureCube irradiance : register(t0);

Texture2D baseColourTex : register(t3);
Texture2D metallicRoughnessTex : register(t4);

void getMaterialProperties(out float3 baseColour, out float roughness, out float metallic, in float2 coord)
{
    baseColour = material.hasBaseColourTex ? baseColourTex.Sample(bilinearWrap, coord).rgb * material.baseColour.rgb : material.baseColour.rgb;
    float2 metallicRoughness = material.hasMetallicRoughnessTex ? metallicRoughnessTex.Sample(bilinearWrap, coord).bg * float2(material.metallicFactor, material.roughnessFactor) : float2(material.metallicFactor, material.roughnessFactor);

    metallic = metallicRoughness.x;
    roughness = metallicRoughness.y; // * metallicRoughness.y; // Perceptural roughness
}

float4 Exercise11PS(float3 positionWS : POSITION, float3 normalWS : NORMAL, float2 texCoord : TEXCOORD) : SV_TARGET
{
    float3 baseColour;
    float roughness;
    float metallic;
    getMaterialProperties(baseColour, roughness, metallic, texCoord);

    float3 N = normalize(normalWS);
    float3 V = normalize(viewPos - positionWS);
    float3 R = reflect(-V, N);

    float NdotV = saturate(dot(N, V));

    float3 colour = getDiffuseAmbientLight(N, baseColour, irradiance);

    float3 ldr = PBRNeutralToneMapping(colour);
    
    return float4(linearTosRGB(ldr), 1.0);
}