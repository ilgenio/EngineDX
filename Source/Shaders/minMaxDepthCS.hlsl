#define TILE_GROUP_SIZE 8

cbuffer MinMaxDepthData : register(b0)
{
    int2 size;
    bool inputIsDepth; // Whether the input texture is a depth texture (single channel) or a min-max texture (two channels)
};

Texture2D<float> inputDepth : register(t0);
Texture2D<float2> inputMinMax : register(t1);
Texture2D<float2> input : register(t0);

RWTexture2D<float2> output : register(u0); // R = min depth, G = max depth

shared uint minDepth;
shared uint maxDepth;

[numthreads(TILE_GROUP_SIZE, TILE_GROUP_SIZE, 1)]
void main(uint2 globalIdx : SV_DispatchThreadID, uint localIndex : SV_GroupIndex)
{
    if(globalIdx.x < size.x && globalIdx.y < size.y)
    {
        // Initialize shared variables
         if(localIndex == 0)
        {
            minDepth = 0xffffffff;  
            maxDepth = 0;
        }

        GroupMemoryBarrierWithGroupSync();

        if(inputIsDepth)
        {
            float d = inputDepth.Load(int3(globalIdx, 0));
            uint dInt = asuint(d);
            InterlockedMin(minDepth, dInt);
            InterlockedMax(maxDepth, dInt);
        }
        else
        {
            float2 d = inputMinMax.Load(int3(globalIdx, 0));
            uint minDInt = asuint(d.x);
            uint maxDInt = asuint(d.y);
            InterlockedMin(minDepth, minDInt);
            InterlockedMax(maxDepth, maxDInt);
        }

        GroupMemoryBarrierWithGroupSync();

        if(localIndex == 0)
        {
            output[globalIdx] = float2(asfloat(minDepth), asfloat(maxDepth));
        }
    }
}