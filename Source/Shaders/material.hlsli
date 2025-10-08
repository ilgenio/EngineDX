#ifndef _MATERIAL_HLSLI_
#define _MATERIAL_HLSLI_

#include "samplers.hlsli"

// Bitfield flag: indicates presence of a base colour texture
#define HAS_BASECOLOUR_TEX        0x1  // 0x1 : hasBaseColourTex
// Bitfield flag: indicates presence of a metallic-roughness texture
#define HAS_METALLICROUGHNESS_TEX 0x2  // 0x2 : hasMetallicRoughnessTex

#define HAS_OCCLUSION_TEX       0x4  // 0x4 : hasOcclusionTex

// Material structure for PBR shading.
// Contains base colour, metallic/roughness factors, normal/occlusion/alpha parameters, and bitfield flags.
struct Material
{
    float4 baseColour;         // Base colour (albedo) of the material
    float  metallicFactor;     // Scalar metallic factor (0 = dielectric, 1 = metal)
    float  roughnessFactor;    // Scalar roughness factor (0 = smooth, 1 = rough)
    float  normalScale;        // Scale for normal map intensity
    float  alphaCutoff;        // Alpha cutoff threshold for alpha masking
    float  occlusionStrength;  // Occlusion map strength
    uint   flags;              // Bitfield flags indicating which textures are present
    uint   padding[2];         // Padding to align struct to 16 bytes
};

float computeSpecularAO(float NdotV, float ao, float roughness) 
{
    return clamp(pow(NdotV + ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + ao, 0.0, 1.0);
}

void getAmbientOcclusion(in Material material, in Texture2D occlusionTex, in float2 coord, in float NdotV, in float roughness, 
                         out float diffuseAO, out float specularAO)
{
    if (material.flags & HAS_OCCLUSION_TEX)
    {
        diffuseAO = occlusionTex.Sample(bilinearWrap, coord).r * material.occlusionStrength;
        specularAO = computeSpecularAO(NdotV, diffuseAO, roughness);
    }
    else
    {
        diffuseAO = 1.0;
        specularAO = 1.0;
    }
}

// Computes the final base colour, roughness, alpha roughness, and metallic values for a material,
// sampling from textures if the corresponding flags are set.
// - material: Material parameters and flags
// - baseColourTex: Base colour texture (if present)
// - metallicRoughnessTex: Metallic-roughness texture (if present)
// - coord: Texture coordinates
// - baseColour: Output final base colour (RGB)
// - roughness: Output final roughness value
// - alphaRoughness: Output perceptual roughness (roughness squared)
// - metallic: Output final metallic value
void getMetallicRoughness(in Material material, in Texture2D baseColourTex, in Texture2D metallicRoughnessTex,
                          in float2 coord, out float3 baseColour, out float roughness, 
                          out float alphaRoughness, out float metallic)
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
    alphaRoughness = roughness * roughness; // Perceptual roughness
}

#endif // _MATERIAL_HLSLI_