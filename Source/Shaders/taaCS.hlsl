#include "common.hlsli"
#include "samplers.hlsli"

cbuffer Constants : register(b0)
{
    float4x4 projection;
    float4x4 invView;
    float4x4 prevViewProj;

    float2 jitter;

    uint width;
    uint height;

    uint prevWidth;
    uint prevHeight;
};

Texture2D<float4> current : register(t0);
Texture2D<float> depth : register(t1);

RWTexture2D<float4> taaResult : register(u0);

float2 reproject(float2 uv, float depth)
{
    float3 worldPos = reconstructWorldPosition(uv, depth, projection, invView);

    // Project world position into previous frame
    float4 prevClipPos = mul(float4(worldPos, 1.0), prevViewProj);
    prevClipPos /= prevClipPos.w; // Perspective divide

    // Convert to UV coordinates
    return ndcToUV(prevClipPos.xy);
}

[numthreads(8, 8, 1)]
void main(uint2 globalIdx : SV_DispatchThreadID)
{
    if(globalIdx.x >= width || globalIdx.y >= height)
    {
        return;
    }
    
    float currentDepth = depth[globalIdx].r;

    float2 uv = (float2(globalIdx) + float2(0.5, 0.5)) / float2(width, height);

    //uv = uv - jitter; // remove jitter for reprojection

    float2 prevUV = reproject(uv, currentDepth);    

    if(prevUV.x < 0.0 || prevUV.x > 1.0 || prevUV.y < 0.0 || prevUV.y > 1.0)
    {
        // Outside the previous frame's bounds, use current color
        taaResult[globalIdx] = current[globalIdx];
        return;
    }

    uint2 prevPos = uint2(prevUV * float2(prevWidth, prevHeight));
    
    float3 prevColor = taaResult[prevPos].rgb;    
    float3 currentColor = current[globalIdx].rgb;

    // colour clampling

    float3 minColor = 9999.0;
    float3 maxColor = -9999.0;

    for(int i = -1; i <= 1; ++i)
    {
        for(int j = -1; j <= 1; ++j)
        {
            uint2 offsetPos = prevPos + uint2(i, j);

            if(offsetPos.x < 0 || offsetPos.x >= prevWidth || offsetPos.y < 0 || offsetPos.y >= prevHeight)
            {
                continue;
            }
            float3 sampleColor = current[offsetPos].rgb;

            minColor = min(minColor, sampleColor);
            maxColor = max(maxColor, sampleColor);
        }
    }

    prevColor = clamp(prevColor, minColor, maxColor);

    // TODO: Depth rejection 

    // Blending factor, can be adjusted for more or less smoothing
    const float alpha = 0.1; 

    taaResult[globalIdx] = float4(lerp(prevColor, currentColor, alpha), 1.0);
}