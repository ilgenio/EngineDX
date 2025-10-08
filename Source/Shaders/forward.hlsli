#ifndef _FORWARD_HLSLI_
#define _FORWARD_HLSLI_

#include "common.hlsli"
#include "lights.hlsli"
#include "material.hlsli"

cbuffer MVP : register(b0)
{
    float4x4 mvp;  
};

cbuffer PerFrame : register(b1)
{
    uint numDirLights;      // Number of directional lights
    uint numPointLights;    // Number of point lights 
    uint numSpotLights;     // Number of spot lights
    uint numRoughnessLevels; // Number of roughness levels in the prefiltered environment map

    float3 viewPos;         // Camera position
    uint   pad;              // Padding to ensure 16-byte alignment
};

cbuffer PerInstance : register(b2)
{
    float4x4 modelMat;
    float4x4 normalMat;
    
    Material material;
};


StructuredBuffer<Directional> dirLights : register(t0);
StructuredBuffer<Point> pointLights : register(t1);
StructuredBuffer<Spot>  spotLights  : register(t2);

TextureCube irradiance : register(t3);
TextureCube radiance : register(t4);
Texture2D  brdfLUT : register(t5);

Texture2D baseColourTex : register(t6);
Texture2D metallicRoughnessTex : register(t7);
Texture2D occlusionTex : register(t9);

#endif // _FORWARD_HLSLI_