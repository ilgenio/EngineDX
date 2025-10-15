#include "forward.hlsli"

#include "lighting.hlsli"
#include "tonemap.hlsli"
#include "ibl.hlsli"

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


float4 main(float3 worldPos : POSITION, float2 texCoord : TEXCOORD, float3 normal : NORMAL0, float3 centroidNormal : NORMAL1, float4 tangent : TANGENT) : SV_TARGET
{
    float3 V  = normalize(viewPos - worldPos);
    
    float3 N;
    if(dot(normal, normal) >= 1.01)
    {
        N = normalize(centroidNormal);
    }
    else
    {
        N = normalize(normal);

    }
    
    float3 baseColour;
    float roughness;
    float alphaRoughness;
    float metallic;
    getMetallicRoughness(material, baseColourTex, metallicRoughnessTex, texCoord, baseColour, roughness, alphaRoughness, metallic);

    roughness = getGeometricSpecularAA(N, roughness);
    alphaRoughness = roughness * roughness;

    /*float3 T = normalize(tangent.xyz);
    float3 B = tangent.w*normalize(cross(N, T));    
    N = getNormal(material, normalTex, texCoord, N, T, B);*/

    float NdotV = saturate(dot(N, V));
    float diffuseAO, specularAO;
    getAmbientOcclusion(material, occlusionTex, texCoord, NdotV, alphaRoughness, diffuseAO, specularAO);
        
    // IBL
    float3 colour = computeLighting(V, N, irradiance, radiance, brdfLUT, numRoughnessLevels, baseColour, roughness, metallic, diffuseAO, specularAO);

    // Direct lights
    for (uint i = 0; i < numDirLights; i++)
        colour += computeLighting(V, N, dirLights[i], baseColour, alphaRoughness, metallic);
    
    for (uint i = 0; i < numPointLights; i++) 
        colour += computeLighting(V, N, pointLights[i], worldPos, baseColour, alphaRoughness, metallic);

    for( uint i = 0; i< numSpotLights; i++)
        colour += computeLighting(V, N, spotLights[i], worldPos, baseColour, alphaRoughness, metallic);

    colour += getEmissive(material, emissiveTex, texCoord);

    // tonemapping
    float3 ldr = PBRNeutralToneMapping(colour);
    
    // gamma correction
    return float4(linearTosRGB(ldr), 1.0);
}