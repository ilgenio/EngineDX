#pragma once

#include "ShaderTableDesc.h"

struct RenderData;

// Parallel reduction pass that computes the minimum and maximum depth values in tiles across the screen. 
// This is used for sample distribution shadow maps 
class DepthMinMaxPass
{
    enum RootParams
    {
        ROOTPARAM_CONSTANTS = 0,
        ROOTPARAM_INPUT_DEPTH,
        ROOTPARAM_INPUT_MINMAX,
        ROOTPARAM_OUTPUT,
        ROOTPARAM_COUNT
    };

    struct Constants
    {
        UINT width;
        UINT height;
        BOOL inputIsDepth;
        UINT padding; // Padding to make the size of the constants buffer a multiple of 16 bytes
    };

    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;

    ComPtr<ID3D12Resource> depthMinMaxTexture[2]; // ping-pong textures for compute shader output
    ShaderTableDesc shaderTableDesc;

    UINT width = 0;
    UINT height = 0;
    UINT finalSrvIndex = 0; // The index of the SRV that contains the final min-max result after reduction

public:

    DepthMinMaxPass();
    ~DepthMinMaxPass();

    void resize(UINT width, UINT height);
    void record(ID3D12GraphicsCommandList* commandList, const RenderData& renderData);
private:
    bool createRootSignature();
    bool createPSO();
};