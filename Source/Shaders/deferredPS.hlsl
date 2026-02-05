
#include "common.hlsli"
#include "lights.hlsli"
#include "lighting.hlsli"
#include "samplers.hlsli"
#include "material.hlsli"
#include "tonemap.hlsli"
#include "ibl.hlsli"

cbuffer PerFrame : register(b0)
{
    uint numDirLights;              // Number of directional lights
    uint numPointLights;            // Number of point lights 
    uint numSpotLights;             // Number of spot lights
    uint numRoughnessLevels;        // Number of roughness levels in the prefiltered environment map

    float3      viewPos;            // Camera position
    uint        pad;

    float4x4    projection;         // projection matrix
    float4x4    invView;            // Inverse view matrix
};

Texture2D gBufferAlbedo : register(t0);
Texture2D gBufferNormalMetRoug : register(t1);
Texture2D gBufferEmissiveOccl : register(t2);
Texture2D depthTex : register(t3);

StructuredBuffer<Directional> dirLights : register(t4);
StructuredBuffer<Point> pointLights : register(t5);
StructuredBuffer<Spot>  spotLights  : register(t6);

TextureCube irradiance : register(t7);
TextureCube radiance : register(t8);
Texture2D  brdfLUT : register(t9);

float4 main(in float2 uv : TEXCOORD) : SV_Target
{
    // Retrieve data from G-buffer
    float4 albedoData = gBufferAlbedo.Sample(pointClamp, uv);
    float4 normalMetRougData = gBufferNormalMetRoug.Sample(pointClamp, uv);
    float4 emissiveOcclData = gBufferEmissiveOccl.Sample(pointClamp, uv);

    float3 baseColour = albedoData.rgb;
    float metallic = f16tof32(asuint(normalMetRougData.a) & 0xFFFF);
    float roughness = f16tof32((asuint(normalMetRougData.a) >> 16) & 0xFFFF);
    float alphaRoughness = roughness * roughness;

    float3 N = normalize(normalMetRougData.xyz);
    float3 emissive = emissiveOcclData.rgb;
    float diffuseAO = emissiveOcclData.a;

    float depth = depthTex.Sample(pointClamp, uv).r;
    float3 worldPos = reconstructWorldPosition(uv, depth, projection, invView);
    float3 V = normalize(viewPos - worldPos);

    float3 R = reflect(-V, N);
    float NdotV = saturate(dot(N, V));
    float NdotR = saturate(dot(N, R));

    float specularAO = getSpecularAO(NdotV, NdotR, diffuseAO, roughness);

    // Compute lighting

    // IBL
    float3 colour = computeLighting(V, N, irradiance, radiance, brdfLUT, numRoughnessLevels, 
                                    baseColour, roughness, metallic, diffuseAO, specularAO);

    // Direct lights
    for (uint i = 0; i < numDirLights; i++)
        colour += computeLighting(V, N, dirLights[i], baseColour, alphaRoughness, metallic);
    
    for (uint i = 0; i < numPointLights; i++) 
        colour += computeLighting(V, N, pointLights[i], worldPos, baseColour, alphaRoughness, metallic);

    for( uint i = 0; i< numSpotLights; i++)
        colour += computeLighting(V, N, spotLights[i], worldPos, baseColour, alphaRoughness, metallic);

    colour += emissive;

    // tonemapping
    float3 ldr = PBRNeutralToneMapping(colour);
    
    // gamma correction
    return float4(linearTosRGB(ldr), 1.0);
}