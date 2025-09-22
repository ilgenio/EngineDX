#include "Common.hlsli"

Texture2D skybox : register(t0);
SamplerState skyboxSampler : register(s0);

float2 CartesianToEquirectangular(in float3 dir)
{
    float phi;

    phi = atan2(dir.z, dir.x); // between -PI , PI
    phi = 1.0-phi/(2.0*PI)+0.5;

    float theta = asin(dir.y);  // between -PI/2 ,  PI/2
    theta = theta/PI+0.5;

    return float2(1.0-phi, 1.0-theta);
}

float4 HDRToCubemapPS(float3 coords : TEXCOORD) : SV_Target
{
    float3 dir = normalize(coords);
    float2 uv  = CartesianToEquirectangular(dir);
    return float4(skybox.Sample(skyboxSampler, uv).rgb, 1.0);
}