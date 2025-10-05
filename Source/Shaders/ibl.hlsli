#ifndef ibl_hlsli
#define ibl_hlsli    

#include "common.hlsli"
#include "samplers.hlsli"

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

float3 getDiffuseAmbientLight(float3 N, float3 baseColour, TextureCube irradianceMap)
{
    return irradianceMap.Sample(bilinearClamp, N).rgb * baseColour;
}

void getSpecularAmbientLightNoFresnel(in float3 R, in float NdotV, in float roughness, in float roughnessLevels, in TextureCube prefilteredEnvMap, 
                                      in Texture2D brdfLUT, out float3 firstTerm, out float3 secondTerm)
{
    float3 radiance = prefilteredEnvMap.SampleLevel(bilinearClamp, R, roughness * (roughnessLevels - 1)).rgb;
    float2 fab = brdfLUT.Sample(bilinearClamp, float2(NdotV, roughness)).rg;
    
    firstTerm = radiance * fab.x;
    secondTerm = radiance * fab.y;
}

float3 computeLighting(in float3 V, in float3 N, in TextureCube irradiance, in TextureCube prefilteredEnv, in Texture2D brdfLUT, in float roughnessLevels, 
                       in float3 baseColour, in float roughness, in float metallic)
{
    float3 R  = reflect(-V, N);
    float NdotV = saturate(dot(N, V));

    float3 diffuse = getDiffuseAmbientLight(N, baseColour, irradiance);

    float3 firstTerm, secondTerm;
    getSpecularAmbientLightNoFresnel(R, NdotV, roughness, roughnessLevels, prefilteredEnv, brdfLUT, firstTerm, secondTerm);
    
    float3 metal_specular = baseColour*firstTerm + secondTerm;    
    float3 dielectric_specular = 0.04*firstTerm + secondTerm;
    
    return lerp(diffuse+dielectric_specular, metal_specular, metallic);
}
    
#endif /* ibl_hlsli */