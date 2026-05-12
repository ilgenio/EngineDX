#include "common.hlsli"

cbuffer BuildMVPData : register(b0)
{
    float4x4 projection;
    float4x4 invView;
    float3   lightDir;
    float aspectRatio;
    float fov;
};

#define SUN_DISTANCE 50.0

Texture2D<float2> inputMinMax : register(t0);
RWStructuredBuffer<float4x4> outputVP : register(u0);

void getCornerPoints(out float3 points[8], in float4x4 proj, in float4x4 invView)
{
    float3 ndcCorners[8] =
    {
        float3(-1, 1, 0),
        float3(1, 1, 0),
        float3(1, -1, 0),
        float3(-1, -1, 0),
        float3(-1, 1, 1),
        float3(1, 1, 1),
        float3(1, -1, 1),
        float3(-1, -1, 1)
    };

    for(int i = 0; i < 8; ++i)
    {
        float2 uv = ndcToUV(ndcCorners[i].xy);
        float depth = ndcCorners[i].z;

        points[i] = reconstructWorldPosition(uv, depth, proj, invView);
    }
}

float4x4 getPerspectiveProj(float aspect, float fov, float nearPlane, float farPlane)
{
    float yScale = 1.0 / tan(fov * 0.5);
    float xScale = yScale / aspect;
    float fRange = farPlane / (nearPlane - farPlane);

    return float4x4(
        xScale, 0, 0, 0,
        0, yScale, 0, 0,
        0, 0, fRange, -1,
        0, 0, fRange * nearPlane, 0
    );
}

float4x4 getOrthographicProj(float width, float height, float nearPlane, float farPlane)
{
    float fRange = 1.0 / (nearPlane - farPlane);

    return float4x4(
        2.0 / width, 0, 0, 0,
        0, 2.0 / height, 0, 0,
        0, 0, fRange, 0,
        0, 0, fRange * nearPlane, 1
    );
}

float4 sphereFromPoints(float3 points[8])
{
    float3 center = float3(0, 0, 0);
    for(int i = 0; i < 8; ++i)
    {
        center += points[i];
    }

    center /= 8.0;

    float radius = 0;
    for(int i = 0; i < 8; ++i)
    {
        float dist = length(points[i] - center);
        radius = max(radius, dist);
    }

    return float4(center, radius);
}

float4x4 getView(float3 eye, float3 target, float3 up)
{
    float3 zAxis = normalize(target - eye);
    float3 xAxis = normalize(cross(up, zAxis));
    float3 yAxis = cross(zAxis, xAxis);

    return float4x4(
        xAxis.x, yAxis.x, -zAxis.x, 0,
        xAxis.y, yAxis.y, -zAxis.y, 0,
        xAxis.z, yAxis.z, -zAxis.z, 0,
        -dot(xAxis, eye), -dot(yAxis, eye), dot(zAxis, eye), 1
    );
}

[numthreads(1, 1, 1)]
void main()
{
    float2 minMaxDepth = inputMinMax.Load(int3(0, 0, 0));

    minMaxDepth.y = max(minMaxDepth.y, minMaxDepth.x + 0.0001); // Ensure far plane is always greater than near plane

    float near = -lineariseDepth(minMaxDepth.x, projection);
    float far = -lineariseDepth(minMaxDepth.y, projection);

    float4x4 clampedProj = getPerspectiveProj(aspectRatio, fov, near, far);

    float3 cornerPoints[8];
    getCornerPoints(cornerPoints, clampedProj, invView);

    float4 sphere = sphereFromPoints(cornerPoints);

    // Compute new view matrix 
    float3 eye = sphere.xyz - lightDir * (sphere.w + SUN_DISTANCE);
    float3 up = float3(0, 1, 0);

    float4x4 view = getView(eye, sphere.xyz, up);

    outputVP[0] = mul(view, getOrthographicProj(sphere.w * 2.0, sphere.w * 2.0, 0.0, sphere.w*2 + SUN_DISTANCE )); 
}