#pragma once

#include "GBuffer.h"


// The common per-frame data structure that is sent to shaders. Contains camera info, light counts, and other global parameters.
struct PerFrame
{
    UINT numDirectionalLights = 0;
    UINT numPointLights = 0;
    UINT numSpotLights = 0;
    UINT numRoughnessLevels = 0;

    UINT width;
    UINT height;

    UINT pad0[2];

    Vector3 cameraPosition;
    UINT pad1; // Padding to ensure 16-byte alignment

    Matrix proj;
    Matrix invView;
};

// Structure containing GPU addresses of the light data buffers. This is passed to the deferred shader to access light information.
struct LightsData
{
    D3D12_GPU_VIRTUAL_ADDRESS directionalLightsAddress;
    D3D12_GPU_VIRTUAL_ADDRESS pointLightsAddress;
    D3D12_GPU_VIRTUAL_ADDRESS spotLightsAddress;
    D3D12_GPU_VIRTUAL_ADDRESS pointLightIndicesAddress;
    D3D12_GPU_VIRTUAL_ADDRESS spotLightIndicesAddress;
};

// The main structure that encapsulates all the data needed for rendering. This is passed to the render passes to access per-frame data, skinning results, light information, and descriptor tables.
struct RenderData
{
    UINT                        width; 
    UINT                        height;
    Matrix                      view;
    Matrix                      invView;
    Matrix                      proj;
    Matrix                      viewProj; 

    D3D12_GPU_VIRTUAL_ADDRESS   perFrameBuffer;
    D3D12_GPU_VIRTUAL_ADDRESS   skinningBuffer;
    D3D12_GPU_DESCRIPTOR_HANDLE iblTable;

    LightsData                  lightsData;
    GBuffer                     gBuffer;
};
