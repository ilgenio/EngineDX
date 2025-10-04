#ifndef _MATERIAL_HLSLI_
#define _MATERIAL_HLSLI_

#include "samplers.hlsli"

#define HAS_BASECOLOUR_TEX        0x1  // 0x1 : hasBaseColourTex
#define HAS_METALLICROUGHNESS_TEX 0x2  // 0x2 : hasMetallicRoughnessTex

struct Material
{
    float4 baseColour;
    float  metallicFactor;
    float  roughnessFactor;
    float  normalScale;
    float  alphaCutoff;
    float  occlusionStrength;
    uint   flags;                   // Bitfield flags
    uint   padding[2];              // Padding to 16 bytes
};

void getMetallicRoughness(in Material material, in Texture2D baseColourTex, in Texture2D metallicRoughnessTex,
                          in float2 coord, out float3 baseColour, out float roughness, out float alphaRoughness, out float metallic)
{
    baseColour = material.baseColour.rgb;

    if (material.flags & HAS_BASECOLOUR_TEX)
    {
        baseColour *= baseColourTex.Sample(bilinearWrap, coord).rgb;
    }

    float2 metallicRoughness = float2(material.metallicFactor, material.roughnessFactor);

    if (material.flags & HAS_METALLICROUGHNESS_TEX)
    {
        metallicRoughness *= metallicRoughnessTex.Sample(bilinearWrap, coord).bg;
    }

    metallic = metallicRoughness.x;
    roughness = metallicRoughness.y;
    alphaRoughness = roughness * roughness; // Perceptural roughness
}

#endif // _MATERIAL_HLSLI_