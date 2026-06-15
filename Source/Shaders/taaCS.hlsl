#include "tonemap.hlsli"


cbuffer Constants : register(b0)
{
    uint width;
    uint height;
    uint padding0;
    uint padding1;
};

Texture2D<float4> current : register(t0);
RWTexture2D<float4> taaResult : register(u0);

[numthreads(8, 8, 1)]
void main(uint2 globalIdx : SV_DispatchThreadID)
{
    if(globalIdx.x >= width || globalIdx.y >= height)
    {
        return;
    }
    
    float3 prevColor    = taaResult[globalIdx].rgb;
    float3 currentColor = current[globalIdx].rgb;
    
    // Blending factor, can be adjusted for more or less smoothing
    const float alpha = 0.1; 

    taaResult[globalIdx] = float4(lerp(prevColor, currentColor, alpha), 1.0);
}