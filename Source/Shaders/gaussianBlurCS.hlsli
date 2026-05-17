#include "samplers.hlsli"

#ifndef TYPE 
#define TYPE float4
#endif

cbuffer BlurData : register(b0)
{
    int2 direction;
    int2 size;
};

static const int SAMPLE_COUNT = 3;

static const float OFFSETS[SAMPLE_COUNT] = { -1.3446745248463534, 0.4466722983756714, 2 };
static const float WEIGHTS[SAMPLE_COUNT] = { 0.35564374091247164, 0.5217749216739427, 0.1225813374135857 };


Texture2D<TYPE> input : register(t0);
RWTexture2D<TYPE> output : register(u0);

#define GROUP_SIZE_X 8
#define GROUP_SIZE_Y 8


[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void main(uint2 globalIdx : SV_DispatchThreadID, uint localIndex : SV_GroupIndex)
{
    if(globalIdx.x < size.x && globalIdx.y < size.y)
    {
        TYPE result = 0.0;
        float2 invSize = 1.0 / float2(size);

        for(int i = 0; i < SAMPLE_COUNT; i++)
        {
            float2 offset   = direction * float(OFFSETS[i]);
            float2 sampleIdx  = float2(globalIdx) + offset;

            float2 uv = sampleIdx* invSize;

            TYPE sampleValue = input.SampleLevel(bilinearClamp, uv, 0);
            float weight = WEIGHTS[i];

            result += sampleValue * weight;
        }

        output[globalIdx] = result;
    }
}