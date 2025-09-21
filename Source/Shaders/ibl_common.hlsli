#ifndef ibl_common_hlsli
#define ibl_common_hlsli    

#include "common.hlsli"

// https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-20-gpu-based-importance-sampling
// https://cgg.mff.cuni.cz/~jaroslav/papers/2007-sketch-fis/Final_sap_0073.pdf
float computeLod(float pdf, int numSamples, int width)
{
    // Solid angle of current sample -- bigger for less likely samples
    precise float solidAngle = 1.0 / (float(numSamples) * pdf + 1e-6);

    // Solid angle of texel
    precise float texelSolidAngle = 4.0 * PI / (6.0 * width * width);

    // mip level, 0.5*log2 because each mip level is 4x smaller
    return max(0.5*log2(solidAngle/texelSolidAngle), 0.0);
}

float3 getIBLIrradiance(float3 N, TextureCube irradianceMap, SamplerState envSampler)
{
    return irradianceMap.Sample(envSampler, N).rgb;
}

float3 getIBLRadiance(float3 R, float roughness, float roughnessLevels, TextureCube prefilteredEnvMap, SamplerState envSampler)
{
    return prefilteredEnvMap.SampleLevel(envSampler, R, roughness * (roughnessLevels - 1)).rgb;
}

float3 getIBLBRDF(float NdotV, float roughness, float F0, Texture2D brdfLUT, SamplerState brdfSampler)
{
    float2 ab = brdfLUT.Sample(brdfSampler, float2(NdotV, roughness)).rg;
    return F0 * ab.x + ab.y;
}

float3 getIBLBRDF(float NdotV, float roughness, float3 F0, Texture2D brdfLUT, SamplerState brdfSampler)
{
    float2 ab = brdfLUT.Sample(brdfSampler, float2(NdotV, roughness)).rg;
    return F0 * ab.x + ab.y;
}

#endif /* ibl_common_hlsli */