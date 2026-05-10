#pragma once

#include "ShaderTableDesc.h"

struct RenderData;

// Parallel reduction pass that computes the minimum and maximum depth values in tiles across the screen. 
// This is used for sample distribution shadow maps 
class DepthMinMaxPass
{
    enum MinMaxRootParams
    {
        MINMAX_ROOTPARAM_CONSTANTS = 0,
        MINMAX_ROOTPARAM_INPUT_DEPTH,
        MINMAX_ROOTPARAM_INPUT_MINMAX,
        MINMAX_ROOTPARAM_OUTPUT,
        MINMAX_ROOTPARAM_COUNT
    };

    enum BuildRootParams
    {
        BUILD_ROOTPARAM_CONSTANTS = 0,
        BUILD_ROOTPARAM_INPUT_MINMAX,
        BUILD_ROOTPARAM_OUTPUT_VP,
        BUILD_ROOTPARAM_COUNT
    };

    struct MinMaxConstants
    {
        UINT width;
        UINT height;
        BOOL inputIsDepth;
        UINT padding; // Padding to make the size of the constants buffer a multiple of 16 bytes
    };

    struct BuildConstants
    {
        Matrix  projection;
        Matrix  invView;
        Vector3 lightDir;
        float   aspectRatio;
        float   fov;
        float   padding[3]; // Padding to make the size of the constants buffer a multiple of 16 bytes
    };

    ComPtr<ID3D12RootSignature> minMaxRS;
    ComPtr<ID3D12PipelineState> minMaxPSO;

    ComPtr<ID3D12RootSignature> buildRS;
    ComPtr<ID3D12PipelineState> buildPSO;
    
    ComPtr<ID3D12Resource> vpBuffer; 
    ComPtr<ID3D12Resource> depthMinMaxTexture[2]; // ping-pong textures for compute shader output
    ShaderTableDesc shaderTableDesc;

    UINT width = 0;
    UINT height = 0;
    UINT finalSrvIndex = 0; // The index of the SRV that contains the final min-max result after reduction

public:

    DepthMinMaxPass();
    ~DepthMinMaxPass();

    void resize(UINT width, UINT height);
    void record(ID3D12GraphicsCommandList* commandList, const Vector3& lightDir, const RenderData& renderData);

    D3D12_GPU_VIRTUAL_ADDRESS getVPBufferAddress() const { return vpBuffer->GetGPUVirtualAddress(); }

private:
    bool createMinMaxRS();
    bool createMinMaxPSO();

    bool createBuildRS();
    bool createBuildPSO();
};