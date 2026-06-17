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
    D3D12_GPU_VIRTUAL_ADDRESS directionalLightsAddress = 0;
    D3D12_GPU_VIRTUAL_ADDRESS pointLightsAddress = 0;
    D3D12_GPU_VIRTUAL_ADDRESS spotLightsAddress = 0;
    D3D12_GPU_VIRTUAL_ADDRESS pointLightIndicesAddress = 0;
    D3D12_GPU_VIRTUAL_ADDRESS spotLightIndicesAddress = 0;
};

// The main structure that encapsulates all the data needed for rendering. This is passed to the render passes to access per-frame data, skinning results, light information, and descriptor tables.
struct RenderData
{
    UINT                        width  = 0;
    UINT                        height = 0;
    UINT                        prevWidth = 0;
    UINT                        prevHeight = 0;

    Matrix                      view   = Matrix::Identity;
    Matrix                      invView = Matrix::Identity;
    Matrix                      proj = Matrix::Identity;
    Matrix                      viewProj = Matrix::Identity; 
    Matrix                      prevViewProj = Matrix::Identity;

    D3D12_GPU_VIRTUAL_ADDRESS   perFrameBuffer = 0;
    D3D12_GPU_VIRTUAL_ADDRESS   skinningBuffer = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE iblTable = {};

    D3D12_GPU_VIRTUAL_ADDRESS   shadowViewProjBuffer = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE shadowMapMoments = {};
    D3D12_GPU_DESCRIPTOR_HANDLE ssaoResult = {};

    LightsData                  lightsData;
    GBuffer                     gBuffer;
};
