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

float lineariseDepth(float depth, in float4x4 projection)
{
    float a = projection[3][2];
    float b = projection[2][2];

    return -a / (b + depth); 
}

float3 reconstructViewPosition(float2 uv, float depth, in float4x4 projection)
{
    float zView = lineariseDepth(depth, projection);
    float xView = (uv.x * 2.0 - 1.0) * (-zView) / projection[0][0];
    float yView = ((1.0 - uv.y) * 2.0 - 1.0) * (-zView) / projection[1][1];

    return float3(xView, yView, zView);
}

float3 reconstructWorldPosition(float2 uv, float depth, in float4x4 projection, in float4x4 invView)
{
    float3 viewPos = reconstructViewPosition(uv, depth, projection);
    return mul(float4(viewPos, 1.0), invView).xyz;
}


#endif /* _COMMON_HLSLI_ */