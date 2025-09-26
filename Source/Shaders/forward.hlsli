#ifndef _FORWARD_HLSLI_
#define _FORWARD_HLSLI_

#include "common.hlsli"
#include "lights.hlsli"
#include "material.hlsli"


StructuredBuffer<Directional> dirLights : register(t0);
StructuredBuffer<Point> pointLights : register(t1);
StructuredBuffer<Spot>  spotLights  : register(t2);

Texture2D baseColourTex : register(t3);
Texture2D metallicRoughnessTex : register(t4);
SamplerState materialSamp : register(s0);

cbuffer MVP : register(b0)
{
    float4x4 mvp;  
};

cbuffer PerFrame : register(b1)
{
    uint numDirLights;      // Number of directional lights
    uint numPointLights;    // Number of point lights 
    uint numSpotLights;     // Number of spot lights

    float3 viewPos;         // Camera position
};

cbuffer PerInstance : register(b2)
{
    float4x4 modelMat;
    float4x4 normalMat;
    
    MetallicRoughnessMat material;
};



#endif // _FORWARD_HLSLI_