#include "samplers.hlsli"

#ifndef TYPE 
#define TYPE float4
#endif

#define HALF_KERNEL 5

cbuffer BlurData : register(b0)
{
    int2 direction;
    int width;
    int height;
    int sigma;

    int padding[3];
};

Texture2D<TYPE> input : register(t0);
RWTexture2D<TYPE> output : register(u0);

#define GROUP_SIZE_X 8
#define GROUP_SIZE_Y 8

// TODO: lds optimization by loading the tile into shared memory and then performing the blur on the tile
//#define TILE_SIZE_X GROUP_SIZE_X+HALF_KERNEL*2
//#define TILE_SIZE_Y GROUP_SIZE_Y+HALF_KERNEL*2

//groupshared TYPE tile[TILE_SIZE_X][TILE_SIZE_Y];


[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void main(uint2 globalIdx : SV_DispatchThreadID, uint localIndex : SV_GroupIndex)
{
    if(globalIdx.x < width && globalIdx.y < height)
    {
        TYPE outValue = 0.0;
        float totalWeight = 0.0;

        for(int i = -HALF_KERNEL; i <= HALF_KERNEL; i++)
        {
            int2 offset     = direction * i;
            int2 sampleIdx  = int2(globalIdx) + offset;

            if(sampleIdx.x >= width || sampleIdx.y >= height || sampleIdx.x < 0 || sampleIdx.y < 0)
                continue;

            float2 uv = (float2(sampleIdx) + 0.5) / float2(width, height);

            // Sample the texture
            TYPE sampleValue = input.SampleLevel(bilinearClamp, uv, 0);

            // Calculate the weight based on the distance from the center
            float weight = exp(-(dot(offset, offset) / (2 * sigma * sigma)));

            outValue += sampleValue * weight;
            totalWeight += weight;
        }

        // Prevent division by zero
        if(totalWeight == 0.0)
            totalWeight = 1.0; 

        output[globalIdx] = outValue / totalWeight;
    }
}