#ifndef _LIGHTS_HLSLI_
#define _LIGHTS_HLSLI_

#include "common.hlsli"

struct Directional
{
    float3 Ld;
    float  intensity;
    float3 Lc;
};

struct Point
{
    float3 Lp;
    float  sqRadius;
    float3 Lc;
    float  intensity;
};

struct Spot
{
    float3 Ld;
    float  sqRadius;
    float3 Lp;
    float  inner;
    float3 Lc;
    float  outter;
    float  intensity;
};

float pointFalloff(float sqDist, float sqRadius)
{
    float num = max(1.0-(sqDist*sqDist)/(sqRadius*sqRadius), 0.0);
    float falloff = (num*num)/(sqDist+1);   

    return falloff;
}

float spotFalloff(float cosDist, float inner, float outer)
{
    float angleAtt = saturate((cosDist - outer) / (inner - outer));

    return angleAtt*angleAtt;
}


#endif // _LIGHTS_HLSLI_