#include "forward.hlsli"

#include "lighting.hlsli"
#include "tonemap.hlsli"

float4 main(float3 worldPos : POSITION, float3 normal : NORMAL, float2 texCoord : TEXCOORD) : SV_TARGET
{
    float3 baseColour;
    float roughness;
    float metallic;
    getMaterialProperties(material, baseColourTex, metallicRoughnessTex, texCoord, baseColour, roughness, metallic);

    float3 V  = normalize(viewPos - worldPos);
    float3 N  = normalize(normal);
    
    float3 colour = 0.0; 

    for (uint i = 0; i < numDirLights; i++)
        colour += computeLighting(V, N, dirLights[i], baseColour, roughness, metallic);
    
    for (uint i = 0; i < numPointLights; i++) 
        colour += computeLighting(V, N, pointLights[i], worldPos, baseColour, roughness, metallic);

    for( uint i = 0; i< numSpotLights; i++)
        colour += computeLighting(V, N, spotLights[i], worldPos, baseColour, roughness, metallic);
    
    float3 ldr = PBRNeutralToneMapping(colour);
    
    return float4(linearTosRGB(ldr), 1.0);
}