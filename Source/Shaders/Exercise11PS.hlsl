#include "ibl_common.hlsli"
#include "Exercise11.hlsli"

TextureCube irradiance : register(t0);
TextureCube radiance : register(t1);
Texture2D  brdfLUT : register(t2);

Texture2D baseColourTex : register(t3);
Texture2D metallicRoughnessTex : register(t4);

SamplerState bilinear : register(s0);

void getMaterialProperties(out float3 baseColour, out float roughness, out float metallic, in float2 coord)
{
    baseColour = material.hasBaseColourTex ? baseColourTex.Sample(bilinear, coord).rgb * material.baseColour.rgb : material.baseColour.rgb;
    float2 metallicRoughness = material.hasMetallicRoughnessTex ? metallicRoughnessTex.Sample(bilinear, coord).bg * float2(material.metallicFactor, material.roughnessFactor) : float2(material.metallicFactor, material.roughnessFactor);

    metallic = metallicRoughness.x;
    roughness = metallicRoughness.y * metallicRoughness.y; // Perceptural roughness
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

    float3 diffuse = getIBLIrradiance(N, irradiance, bilinear) * baseColour;
    float3 specular = getIBLRadiance(R, roughness, roughnessLevels, radiance, bilinear);

    float3 brdf_metal_fresnel = getIBLBRDF(NdotV, roughness, baseColour, brdfLUT, bilinear);
    float3 brdf_dielectric_fresnel = getIBLBRDF(NdotV, roughness, 0.04, brdfLUT, bilinear);

    float3 dielectric_colour = diffuse + specular * brdf_dielectric_fresnel;
    float3 metal_colour = specular * brdf_metal_fresnel;

    return float4(lerp(dielectric_colour, metal_colour, metallic).rgb, 1.0);
}