#include "common.hlsli"
#include "lights.hlsli"
#include "tileCulling.hlsli"

cbuffer TileCullingData : register(b0)
{
    float4x4 view;
    float4x4 proj;
    
    int width;
    int height;

    int numPointLights;
    int numSpotLights;
};

Texture2D<float>         depth              : register(t0);
StructuredBuffer<float4> pointCenterRadius  : register(t1);
StructuredBuffer<float4> spotCenterRadius   : register(t2);

RWStructuredBuffer<int> pointList : register(u0);
RWStructuredBuffer<int> spotList : register(u1);

groupshared uint minZ;
groupshared uint maxZ;
groupshared uint pointsInTile;
groupshared uint spotsInTile;
groupshared uint volSpotsInTile;

// Note: Plane is created from 3 points but we assume the other point is (0, 0) because we are in view space
float3 createPlaneEq(in float3 p0, in float3 p1)
{
    float3 N = normalize(cross(p0, p1));

    return N;
}

float getDistanceFromPlane(float3 plane, float3 p)
{
    // plane w is 0
    return dot(plane, p);
}

#define TILE_GROUP_SIZE TILE_RES
#define NUM_THREADS (TILE_GROUP_SIZE*TILE_GROUP_SIZE)

[numthreads(TILE_GROUP_SIZE, TILE_GROUP_SIZE, 1)]
void main(uint2 globalIdx : SV_DispatchThreadID, uint localIndex : SV_GroupIndex)
 {
    if(globalIdx.x < width && globalIdx.y < height)
    {
        float d = depth.Load(int3(globalIdx, 0));

        // Initialize shared variables
         if(localIndex == 0)
        {
            minZ = 0xffffffff;  
            maxZ = 0;
            pointsInTile = 0;
            spotsInTile = 0;
            volSpotsInTile = 0;
        }

        uint2 numTiles = getNumTiles(width, height);

        // Compute tile frustum planes

        uint2 groupIdx = globalIdx / TILE_RES;

        float3 planePoints[4];
        planePoints[0] = reconstructViewPosition(float2(saturate(float(groupIdx.x)*TILE_RES/float(width)), saturate(float(groupIdx.y)*TILE_RES/float(height))), 0.0, proj);    
        planePoints[1] = reconstructViewPosition(float2(saturate(float(groupIdx.x+1)*TILE_RES/float(width)), saturate(float(groupIdx.y)*TILE_RES/float(height))), 0.0, proj);
        planePoints[2] = reconstructViewPosition(float2(saturate(float(groupIdx.x+1)*TILE_RES/float(width)), saturate(float(groupIdx.y+1)*TILE_RES/float(height))), 0.0, proj);
        planePoints[3] = reconstructViewPosition(float2(saturate(float(groupIdx.x)*TILE_RES/float(width)),   saturate(float(groupIdx.y+1)*TILE_RES/float(height))), 0.0, proj);

        float3 planes[4];

        for(int i=0; i< 4; ++i)
        {
            planes[i] = createPlaneEq(planePoints[i], planePoints[(i+1)&3]);
        }

        GroupMemoryBarrierWithGroupSync();

        InterlockedMin(minZ, asuint(d));
        InterlockedMax(maxZ, asuint(d));

        GroupMemoryBarrierWithGroupSync();

        // Do Frustum Culling

        int tileIndex = int(getTileIndex(globalIdx.x, globalIdx.y, width));
        uint threadIndex = localIndex;

        float viewMinZ = lineariseDepth(asfloat(minZ), proj);
        float viewMaxZ = lineariseDepth(asfloat(maxZ), proj);

        // Point lights
        for(uint i=threadIndex;i<numPointLights; i+=NUM_THREADS)
        {
            float4 light = pointCenterRadius[i];
            float radius   = light.w;
            float3 viewPos = mul(float4(light.xyz, 1.0), view).xyz;
    
            if(getDistanceFromPlane(planes[0], viewPos) < radius &&
               getDistanceFromPlane(planes[1], viewPos) < radius &&
               getDistanceFromPlane(planes[2], viewPos) < radius &&
               getDistanceFromPlane(planes[3], viewPos) < radius &&
               (viewMinZ-viewPos.z) > -radius &&
               (viewMaxZ-viewPos.z) < radius)
            {
                uint index;
                
                InterlockedAdd(pointsInTile, 1, index);

                if (index < MAX_LIGHTS_PER_TILE)
                {
                    uint listIndex = tileIndex * MAX_LIGHTS_PER_TILE + index;
                    pointList[int(listIndex)] = i;
                }
            }
        }

        for(uint i=threadIndex;i<numSpotLights; i+=NUM_THREADS)
        {
            float4 light = spotCenterRadius[i];
            float radius   = light.w;
            float3 viewPos = mul(float4(light.xyz, 1.0), view).xyz;
    
            if(getDistanceFromPlane(planes[0], viewPos) < radius &&
               getDistanceFromPlane(planes[1], viewPos) < radius &&
               getDistanceFromPlane(planes[2], viewPos) < radius &&
               getDistanceFromPlane(planes[3], viewPos) < radius &&
               (viewMaxZ-viewPos.z) < radius && 
               (viewMinZ-viewPos.z) > -radius )
            {

                uint index;
                
                InterlockedAdd(spotsInTile, 1, index);

                if (index < MAX_LIGHTS_PER_TILE)
                {
                    uint listIndex = tileIndex * MAX_LIGHTS_PER_TILE + index;
                    spotList[int(listIndex)] = i;
                }
            }
        }

        GroupMemoryBarrierWithGroupSync();

        // Mark last with a -1 so we know where the end of the list is when we read it in the shader
        if(localIndex == 0)
        {
            if (pointsInTile < MAX_LIGHTS_PER_TILE)
            {
                uint lastIndex = tileIndex * MAX_LIGHTS_PER_TILE + pointsInTile;
                pointList[lastIndex] = -1;
            }

            if (spotsInTile < MAX_LIGHTS_PER_TILE)
            {
                uint lastIndex = tileIndex * MAX_LIGHTS_PER_TILE + spotsInTile;
                spotList[lastIndex] = -1;
            }
        }
    }
}