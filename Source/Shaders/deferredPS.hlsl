
#include "common.hlsli"
#include "lights.hlsli"
#include "lighting.hlsli"
#include "samplers.hlsli"
#include "material.hlsli"
#include "tonemap.hlsli"
#include "ibl.hlsli"
#include "tileCulling.hlsli"

cbuffer PerFrame : register(b0)
{
    uint numDirLights; // Number of directional lights
    uint numPointLights; // Number of point lights 
    uint numSpotLights; // Number of spot lights
    uint numRoughnessLevels; // Number of roughness levels in the prefiltered environment map

    uint width; // Viewport width
    uint height; // Viewport height

    float3 viewPos; // Camera position

    float4x4 proj; // projection matrix
    float4x4 invView; // Inverse view matrix

    float4x4 shadowViewProj; // directional light's shadow view-projection matrix
};

Texture2D gBufferAlbedo : register(t0);
Texture2D gBufferNormalMetRoug : register(t1);
Texture2D gBufferEmissiveOccl : register(t2);
Texture2D depthTex : register(t3);

StructuredBuffer<Directional> dirLights : register(t4);
StructuredBuffer<Point> pointLights : register(t5);
StructuredBuffer<Spot>  spotLights  : register(t6);

StructuredBuffer<int> pointLightIndices : register(t7);
StructuredBuffer<int> spotLightIndices : register(t8);

Texture2D shadowMap : register(t12);

TextureCube irradiance : register(t9);
TextureCube radiance : register(t10);
Texture2D  brdfLUT : register(t11);

float3 computeShadowCoord(in float3 worldPos)
{
    float4 shadowPos = mul(float4(worldPos, 1.0), shadowViewProj);
    shadowPos /= shadowPos.w;

    // Transform from NDC to UV space
    shadowPos.xy = ndcToUV(shadowPos.xy); 

    return shadowPos.xyz;
}

float inShadow(in float3 worldPos)
{
    float3 shadowNDC = computeShadowCoord(worldPos);
    
    // If outside of shadow map, consider it lit
    if (shadowNDC.x < 0.0 || shadowNDC.x > 1.0 || shadowNDC.y < 0.0 || shadowNDC.y > 1.0 || shadowNDC.z < 0.0 || shadowNDC.z > 1.0)
        return 0.0;

    float closestDepth = shadowMap.Sample(bilinearClamp, shadowNDC.xy).r;
    float currentDepth = shadowNDC.z;

    // Bias to prevent shadow acne
    float bias = 0.0001; // TODO: pass bias as a PerFrame variable and adjust based on light angle and scene scale

    return currentDepth - bias > closestDepth ? 0.0 : 1.0;
}

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
    float3 worldPos = reconstructWorldPosition(uv, depth, proj, invView);
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
    {
        if (i == 0)
        {
            // TODO: Add to forward rendering pass 
            float shadow = inShadow(worldPos);
            colour += shadow*computeLighting(V, N, dirLights[i], baseColour, alphaRoughness, metallic);
        }
        else 
        {
            colour += computeLighting(V, N, dirLights[i], baseColour, alphaRoughness, metallic);
        }
    }

    // Tiled lights

    uint tileIndex = getTileIndex(uint(uv.x*width), uint(uv.y*height), width);

    // Point lights
    for (uint i = 0; i < MAX_LIGHTS_PER_TILE; i++)
    {
        uint index = tileIndex*MAX_LIGHTS_PER_TILE + i;
        int pointLightIndex = pointLightIndices[index];

        if(pointLightIndex == -1)
        {
            break;
        }

        colour += computeLighting(V, N, pointLights[pointLightIndex], worldPos, baseColour, alphaRoughness, metallic);
    }
    
    // Spot lights
    for (uint i = 0; i < MAX_LIGHTS_PER_TILE; i++)
    {
        int spotLightIndex = spotLightIndices[tileIndex*MAX_LIGHTS_PER_TILE + i];

        if(spotLightIndex == -1)
        {
            break;
        }

        colour += computeLighting(V, N, spotLights[spotLightIndex], worldPos, baseColour, alphaRoughness, metallic);
    }

    colour += emissive;

    // tonemapping
    float3 ldr = PBRNeutralToneMapping(colour);
    
    // gamma correction
    return float4(linearTosRGB(ldr), 1.0);
}