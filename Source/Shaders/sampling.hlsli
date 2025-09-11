#ifndef _SAMPLING_HLSLI
#define _SAMPLING_HLSLI

#include "common.hlsli"

float radicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 hammersley2D(uint i, uint N) 
{
    return float2(float(i)/float(N), radicalInverse_VdC(i));
}

float3 cosineSample(float u1, float u2)
{
    float phi = 2.0 * PI * u1;
    float r = sqrt(u2);

    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(1.0 - u2);

    return float3(x, y, z);
}

float3 ggxSample(in float2 rand, float roughness)
{
    float a = roughness*roughness;
    float phi = 2.0*PI*rand.x;
    float cos_theta = sqrt((1.0-rand.y)/(rand.y*(a*a-1)+1));
    float sin_theta = sqrt(1-cos_theta*cos_theta);

    // spherical to cartesian conversion
    float3 dir;
    dir.x = cos(phi)*sin_theta;
    dir.y = sin(phi)*sin_theta;
    dir.z = cos_theta;

    return dir;
}

#endif // _SAMPLING_HLSLI