#include "forward.hlsli"

#include "lighting.hlsli"
#include "tonemap.hlsli"
#include "ibl.hlsli"
#include "tileCulling.hlsli"

#define VARIANCE 0.3
#define THRESHOLD 0.2

float getGeometricSpecularAA(float3 N, float roughness)
{
    float3 ndx = ddx(N);
    float3 ndy = ddy(N);

    float curvature = max(dot(ndx, ndx), dot(ndy, ndy));

    float geomRoughnessOffset = pow(curvature, 0.333) * VARIANCE;
     
    geomRoughnessOffset = min(geomRoughnessOffset, THRESHOLD);

    return saturate( roughness + geomRoughnessOffset);
}

float4 main(float3 worldPos : POSITION0, float3 ndcPos : POSITION1, float2 texCoord0 : TEXCOORD0, float2 texCoord1 : TEXCOORD1, float3 normal : NORMAL0, float3 tangent : TANGENT) : SV_TARGET
{
    float3 V  = normalize(viewPos - worldPos);   
    float3 N = normalize(normal);
    
    float3 baseColour;
    float roughness;
    float alphaRoughness;
    float metallic;
    float alpha;

    getMetallicRoughness(material, baseColourTex, metallicRoughnessTex, texCoord0, texCoord1, baseColour, roughness, alphaRoughness, metallic, alpha);

    if(alpha < material.alphaCutoff)
    {
        discard;
    }

    roughness = getGeometricSpecularAA(N, roughness);
    alphaRoughness = roughness * roughness;

    float3 T = normalize(tangent);
    float3 B = normalize(cross(N, T));    
    N = getNormal(material, normalTex, texCoord0, texCoord1, N, T, B);

    float NdotV = saturate(dot(N, V));
    float3 R = reflect(-V, N);
    float NdotR = saturate(dot(N, R));
    
    float diffuseAO, specularAO;
    getAmbientOcclusion(material, occlusionTex, texCoord0, texCoord1, NdotV, NdotR, alphaRoughness, diffuseAO, specularAO);
        
    // IBL
    float3 colour = computeLighting(V, N, irradiance, radiance, brdfLUT, numRoughnessLevels, baseColour, 
                                    roughness, metallic, diffuseAO, specularAO);

    // Direct lights
    for (uint i = 0; i < numDirLights; i++)
        colour += computeLighting(V, N, dirLights[i], baseColour, alphaRoughness, metallic);

    uint tileIndex = getTileIndexFromNDC(ndcPos.xy, width, height);

    // Point lights    
    for (uint i = 0; i < MAX_LIGHTS_PER_TILE; i++)
    {
        int pointLightIndex = pointLightIndices[tileIndex * MAX_LIGHTS_PER_TILE + i];

        if (pointLightIndex == -1)
        {
            break;
        }

        colour += computeLighting(V, N, pointLights[pointLightIndex], worldPos, baseColour, alphaRoughness, metallic);
    }

    // Spot lights
    for (uint i = 0; i < MAX_LIGHTS_PER_TILE; i++)
    {
        int spotLightIndex = spotLightIndices[tileIndex * MAX_LIGHTS_PER_TILE + i];

        if (spotLightIndex == -1)
        {
            break;
        }

        colour += computeLighting(V, N, spotLights[spotLightIndex], worldPos, baseColour, alphaRoughness, metallic);
    }

    colour += getEmissive(material, emissiveTex, texCoord0, texCoord1);

    // tonemapping
    float3 ldr = PBRNeutralToneMapping(colour);
    
    // gamma correction
    return float4(linearTosRGB(ldr), 1.0);
}