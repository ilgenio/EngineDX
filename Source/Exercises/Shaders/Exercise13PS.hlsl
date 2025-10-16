#include "Exercise13.hlsli"
#include "ibl.hlsli"
#include "tonemap.hlsli"

TextureCube irradiance : register(t0);
TextureCube radiance : register(t1);
Texture2D   brdfLUT : register(t2);

Texture2D baseColourTex : register(t3);
Texture2D metallicRoughnessTex : register(t4);
Texture2D occlusionTex : register(t5);
Texture2D emissiveTex : register(t6);
Texture2D normalTex : register(t7);

float computeSpecularAO(float NdotV, float ao, float roughness) 
{
    return saturate(pow(NdotV + ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + ao);
}

void getAmbientOcclusion(in MetallicRoughnessMat material, in float2 coord, in float NdotV, in float NdotR, 
                         in float roughness, out float diffuseAO, out float specularAO)
{
    if (material.hasOcclusionTex)   
    {
        diffuseAO = occlusionTex.Sample(bilinearWrap, coord).r * material.occlusionStrength;
        specularAO = computeSpecularAO(NdotV, diffuseAO, roughness);
    }
    else
    {
        diffuseAO = 1.0;
        specularAO = 1.0;
    }
    
    // Horizon fade for specular AO
    specularAO *= max(1.0 + NdotR, 1.0); 
}

void getEmissiveColour(out float3 emissiveColour, in float2 coord)
{
    emissiveColour = material.hasEmissiveTex ? emissiveTex.Sample(bilinearWrap, coord).rgb * material.emissiveFactor  : material.emissiveFactor;
}

void getMaterialProperties(out float3 baseColour, out float roughness, out float metallic, in float2 coord)
{
    baseColour = material.hasBaseColourTex ? baseColourTex.Sample(bilinearWrap, coord).rgb * material.baseColour.rgb : material.baseColour.rgb;
    float2 metallicRoughness = material.hasMetallicRoughnessTex ? metallicRoughnessTex.Sample(bilinearWrap, coord).bg * float2(material.metallicFactor, material.roughnessFactor) : float2(material.metallicFactor, material.roughnessFactor);

    metallic = metallicRoughness.x;
    roughness = metallicRoughness.y; // * metallicRoughness.y; // Perceptural roughness
}

float3 getNormal(in float2 coord, in float3 normal, in float3 tangent, in float3 bitangent)
{
    float3 normalMap = normalTex.Sample(bilinearWrap, coord).xyz * 2.0 - 1.0;

    normalMap.xy *= material.normalScale;
    normalMap = normalize(normalMap);
        
    // Transform normal from tangent space to world space
    float3x3 TBN = float3x3(tangent, bitangent, normal);
    return mul(normalMap, TBN);
}


float4 Exercise13PS(float3 positionWS : POSITION, float3 normalWS : NORMAL, float3 tangentWS : TANGENT, float2 texCoord : TEXCOORD) : SV_TARGET
{
    float3 baseColour;
    float roughness;
    float metallic;
    getMaterialProperties(baseColour, roughness, metallic, texCoord);

    float3 N = normalize(normalWS);
    float3 V = normalize(viewPos - positionWS);
    
    if(material.hasNormalMap)
    {
        float3 T = normalize(tangentWS);
        float3 B = normalize(cross(N, T));
        N = getNormal(texCoord, N, T, B);
    }

    float3 R = reflect(-V, N);
    float NdotV = saturate(dot(N, V));
    float NdotR = saturate(dot(N, R));

    float diffuseAO, specularAO;
    getAmbientOcclusion(material, texCoord, NdotV, NdotR, roughness, diffuseAO, specularAO);
    
    float3 emissiveColour;
    getEmissiveColour(emissiveColour, texCoord);

    float3 diffuse = getDiffuseAmbientLight(N, baseColour, irradiance);

    float3 firstTerm, secondTerm;
    getSpecularAmbientLightNoFresnel(R, NdotV, roughness, roughnessLevels, radiance, brdfLUT, firstTerm, secondTerm);

    float3 metal_specular = baseColour * firstTerm + secondTerm;
    float3 dielectric_specular = 0.04 * firstTerm + secondTerm;
    
    float3 colour = emissiveColour+lerp(diffuse * diffuseAO + dielectric_specular * specularAO, metal_specular * specularAO, metallic);
    
    float3 ldr = PBRNeutralToneMapping(colour);
    
    return float4(linearTosRGB(ldr), 1.0);
}