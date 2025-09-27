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

float3 getIBLIrradiance(float3 N, TextureCube irradianceMap)
{
    return irradianceMap.Sample(bilinearClamp, N).rgb;
}

float3 getIBLRadiance(float3 R, float roughness, float roughnessLevels, TextureCube prefilteredEnvMap)
{
    return prefilteredEnvMap.SampleLevel(bilinearClamp, R, roughness * (roughnessLevels - 1)).rgb;
}

float3 getIBLBRDF(float NdotV, float roughness, float F0, Texture2D brdfLUT)
{
    float2 ab = brdfLUT.Sample(bilinearClamp, float2(NdotV, roughness)).rg;
    return F0 * ab.x + ab.y;
}

float3 getIBLBRDF(float NdotV, float roughness, float3 F0, Texture2D brdfLUT)
{
    float2 ab = brdfLUT.Sample(bilinearClamp, float2(NdotV, roughness)).rg;
    return F0 * ab.x + ab.y;
}


float3 computeLighting(in float3 V, in float3 N, in TextureCube irradiance, in TextureCube prefilteredEnv, in Texture2D brdfLUT, in float roughnessLevels, 
                       in float3 baseColour, in float roughness, in float metallic)
{
    float3 R  = reflect(-V, N);
    float NdotV = saturate(dot(N, V));

    float3 diffuse = getIBLIrradiance(N, irradiance) * baseColour;

    float3 colour = diffuse;
    float3 specular = getIBLRadiance(R, roughness, roughnessLevels, prefilteredEnv);

    float3 brdf_metal_fresnel = getIBLBRDF(NdotV, roughness, baseColour, brdfLUT);
    float3 brdf_dielectric_fresnel = getIBLBRDF(NdotV, roughness, 0.04, brdfLUT);

    float3 dielectric_colour = diffuse + specular * brdf_dielectric_fresnel;
    float3 metal_colour = specular * brdf_metal_fresnel;

    colour = lerp(dielectric_colour, metal_colour, metallic);
     
    return colour;
}
    
#endif /* ibl_hlsli */