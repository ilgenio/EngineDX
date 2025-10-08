#ifndef ibl_hlsli
#define ibl_hlsli    

#include "common.hlsli"
#include "samplers.hlsli"

// Computes the mip level (LOD) for importance sampling in environment maps based on the sample PDF, number of samples, and cubemap width.
// Reference: GPU Gems 3, Chapter 20.
float computeLod(float pdf, int numSamples, int width)
{
    // probability of each sample  -- bigger for less likely samples
    precise float solidAngle = 1.0 / (float(numSamples) * pdf + 1e-6);

    // probaby of each texel -- smaller for bigger cubemaps
    precise float texelSolidAngle = 1.0 / (6.0 * width * width);

    // mip level, 0.5*log2 = log4 because each mip level is 4x smaller
    return max(0.5*log2(solidAngle/texelSolidAngle), 0.0);
}

// Computes the diffuse ambient lighting contribution using the irradiance map and base color.
float3 getDiffuseAmbientLight(float3 N, float3 baseColour, TextureCube irradianceMap)
{
    return irradianceMap.Sample(bilinearClamp, N).rgb * baseColour;
}

// Computes the two terms of the split-sum approximation for specular ambient lighting (no Fresnel).
// Uses the prefiltered environment map and BRDF lookup texture.
void getSpecularAmbientLightNoFresnel(in float3 R, in float NdotV, in float roughness, in float roughnessLevels, in TextureCube prefilteredEnvMap, 
                                      in Texture2D brdfLUT, out float3 firstTerm, out float3 secondTerm)
{
    float3 radiance = prefilteredEnvMap.SampleLevel(bilinearClamp, R, roughness * (roughnessLevels - 1)).rgb;
    float2 fab = brdfLUT.Sample(bilinearClamp, float2(NdotV, roughness)).rg;
    
    firstTerm = radiance * fab.x;
    secondTerm = radiance * fab.y;
}


// Computes the total ambient lighting (diffuse + specular) for PBR using IBL resources.
// Blends between dielectric and metallic responses based on the metallic parameter.
float3 computeLighting(in float3 V, in float3 N, in TextureCube irradiance, in TextureCube prefilteredEnv, in Texture2D brdfLUT, in float roughnessLevels, 
                       in float3 baseColour, in float roughness, in float metallic, in float diffuseAO, in float specularAO)
{
    float3 R  = reflect(-V, N);
    float NdotV = saturate(dot(N, V));

    float3 diffuse = getDiffuseAmbientLight(N, baseColour, irradiance) * diffuseAO;

    float3 firstTerm, secondTerm;
    getSpecularAmbientLightNoFresnel(R, NdotV, roughness, roughnessLevels, prefilteredEnv, brdfLUT, firstTerm, secondTerm);
    
    float3 metal_specular = (baseColour*firstTerm + secondTerm) * specularAO;
    float3 dielectric_specular = (0.04*firstTerm + secondTerm) * specularAO;

    return lerp(diffuse+dielectric_specular, metal_specular, metallic);
}
    
#endif /* ibl_hlsli */