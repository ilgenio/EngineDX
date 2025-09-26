#ifndef _MATERIAL_HLSLI_
#define _MATERIAL_HLSLI_

#include "samplers.hlsli"

struct MetallicRoughnessMat
{
    float4 baseColour;
    float  metallicFactor;
    float  roughnessFactor;
    bool   hasBaseColourTex;
    bool   hasMetallicRoughnessTex;
};

void getMaterialProperties(int MaterialRoughnessMat material, in Texture2D baseColourTex, in Texture2D metallicRoughnessTex,
                           in float2 coord, out float3 baseColour, out float roughness, out float metallic)
{
    baseColour = material.hasBaseColourTex ?  baseColourTex.Sample(bilinearClamp, coord).rgb * material.baseColour.rgb : material.baseColour.rgb;
    float2 metallicRoughness = material.hasMetallicRoughnessTex ? metallicRoughnessTex.Sample(bilinearClamp, coord).bg * float2(material.metallicFactor, material.roughnessFactor) : float2(material.metallicFactor, material.roughnessFactor);

    metallic = metallicRoughness.x;
    roughness = metallicRoughness.y * metallicRoughness.y; // Perceptural roughness
}

#endif // _MATERIAL_HLSLI_