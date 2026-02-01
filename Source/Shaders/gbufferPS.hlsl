#include "gbuffer.hlsli"

struct PSOutput
{
    float4 albedo : SV_Target0;
    float4 normalMetalRough : SV_Target1;
    float4 emissiveAO : SV_Target2;
};

PSOutput main(float3 worldPos : POSITION, float2 texCoord : TEXCOORD, float3 normal : NORMAL0, float3 tangent : TANGENT)
{
    PSOutput output;

    float3 baseColour;
    float roughness;
    float metallic;
    getMetallicRoughness(material, baseColourTex, metallicRoughnessTex, texCoord, baseColour, roughness, metallic);

    float3 N = normalize(normal);
    float3 T = normalize(tangent);
    float3 B = normalize(cross(N, T));    
    N = getNormal(material, normalTex, texCoord, N, T, B);

    float diffuseAO = getDiffuseAO(material, occlusionTex, texCoord);
    float3 emissive = getEmissive(material, emissiveTex, texCoord);

    uint metallic16  = f32tof16(metallic);
    uint roughness16 = f32tof16(roughness);

    output.albedo = float4(baseColour, 1.0);
    output.normalMetalRough = float4(N, asfloat(metallic16 | roughness16 << 16));
    output.emissiveAO = float4(emissive, diffuseAO);

    return output;
}