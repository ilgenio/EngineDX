#include "Exercise11.hlsli"
#include "ibl_common.hlsli"
#include "tonemap.hlsli"

TextureCube irradiance : register(t0);
TextureCube radiance : register(t1);
Texture2D  brdfLUT : register(t2);

Texture2D baseColourTex : register(t3);
Texture2D metallicRoughnessTex : register(t4);

SamplerState bilinearWrap : register(s0);
SamplerState pointWrap : register(s1);
SamplerState bilinearClamp : register(s2);
SamplerState pointClamp : register(s3);


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

    float3 diffuse = getIBLIrradiance(N, irradiance, bilinearWrap) * baseColour;
    float3 specular = getIBLRadiance(R, roughness, roughnessLevels, radiance, bilinearWrap);

    float3 brdf_metal_fresnel = getIBLBRDF(NdotV, roughness, baseColour, brdfLUT, bilinearClamp);
    float3 brdf_dielectric_fresnel = getIBLBRDF(NdotV, roughness, 0.04, brdfLUT, bilinearClamp);

    float3 dielectric_colour = diffuse + specular * brdf_dielectric_fresnel;
    float3 metal_colour = specular * brdf_metal_fresnel;

    float3 colour = lerp(dielectric_colour, metal_colour, metallic);

    float3 ldr = PBRNeutralToneMapping(colour);
    
    return float4(linearTosRGB(ldr), 1.0);
}