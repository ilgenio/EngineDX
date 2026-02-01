#ifndef _GBUFFER_HLSLI_
#define _GBUFFER_HLSLI_

#include "common.hlsli"
#include "material.hlsli"

cbuffer MVP : register(b0)
{
    float4x4 mvp;  
};

cbuffer PerInstance : register(b1)
{
    float4x4 modelMat;
    float4x4 normalMat;
    
    Material material;
};

Texture2D baseColourTex : register(t0);
Texture2D metallicRoughnessTex : register(t1);
Texture2D normalTex : register(t2);
Texture2D occlusionTex : register(t3);
Texture2D emissiveTex : register(t4);

#endif // _GBUFFER_HLSLI_
