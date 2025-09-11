#ifndef _COMMON_HLSLI_
#define _COMMON_HLSLI_

#define PI 3.14159265359

float sq(float value)
{
    return value*value;
}

float3 sq(float3 value)
{
    return value*value;
}

float3x3 computeTangetSpace(in float3 normal)
{
    float3 up = abs(normal.y) > 0.999 ? float3(0.0, 0.0, 1.0) : float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, normal));
    up = cross(normal, right);
    
    return float3x3(right, up, normal);
}

#endif /* _COMMON_HLSLI_ */