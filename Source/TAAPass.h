#pragma once

#include "ShaderTableDesc.h"

struct RenderData;

class TAAPass
{
    enum RootParameters
    {
        ROOT_TAA_CONSTANTS = 0,
        ROOT_TAA_PREVIOUS,
        ROOT_TAA_CURRENT,

        NUM_ROOT_PARAMETERS_TAA
    };

    struct TAAConstants
    {
        UINT width;
        UINT height;
        UINT padding0;
        UINT padding1;
    };

    ComPtr<ID3D12PipelineState> pso;
    ComPtr<ID3D12RootSignature> rootSignature;

    ComPtr<ID3D12Resource> taaResult;
    ShaderTableDesc tableDesc;
    
    UINT width = 0;
    UINT height = 0;

public:
    TAAPass();
    ~TAAPass();
    
    void render(ID3D12GraphicsCommandList* commandList, D3D12_GPU_DESCRIPTOR_HANDLE currentFrame);
    void resize(UINT width, UINT height);

    D3D12_GPU_DESCRIPTOR_HANDLE getSrvHandle() const { return tableDesc.getGPUHandle(0); }


private:
    bool createRootSignature();
    bool createPSO();
};