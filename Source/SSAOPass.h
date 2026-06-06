#pragma once

#include "RenderTargetDesc.h"
#include "ShaderTableDesc.h"

struct RenderData;

class SSAOPass
{
    enum RootSlots
    {
        ROOT_PARAMS = 0,
        ROOT_DEPTH_SRV = 1,
        ROOT_NORMAL_SRV = 2,
        ROOT_SAMPLERS = 3,
        ROOT_COUNT
    };

    enum RootParamsBlur
    {
        ROOT_BLUR_CONSTANTS = 0,
        ROOT_BLUR_INPUT,
        ROOT_BLUR_OUTPUT,
        ROOT_BLUR_SAMPLERS,

        NUM_ROOT_PARAMETERS_BLUR
    };

    enum KernelSize
    {
        KERNEL_SIZE = 16
    };

    struct Params
    {
        Matrix proj;
        Vector4 kernel[KERNEL_SIZE]; 
        float kernelRadius;
        float bias;
        UINT frameIndex = 0;
        UINT width;
        UINT height;
        UINT padding[3];
    };

    struct BlurConstants
    {
        int directionX;
        int directionY;
        int width;
        int height;
    };

    ComPtr<ID3D12PipelineState> pso;
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12RootSignature> blurRootSignature;
    ComPtr<ID3D12PipelineState> blurPSO;

    ComPtr<ID3D12Resource> ssaoResult;
    ComPtr<ID3D12Resource> blur0;
    ComPtr<ID3D12Resource> blur1;

    RenderTargetDesc rtv;
    ShaderTableDesc srvDesc;
    UINT width = 0;
    UINT height = 0;
    Params params = {};

public:
    SSAOPass();
    ~SSAOPass();

    void resize(UINT width, UINT height);
    void render(ID3D12GraphicsCommandList* commandList, const RenderData& renderData);

    D3D12_GPU_DESCRIPTOR_HANDLE getSSAOSRV() const { return srvDesc.getGPUHandle(2); }

private:
    bool createRootSignature();
    bool createPSO();
    void createKernel();

    bool createBlurRootSignature();
    bool createBlurPSO();

    void renderSSAO(ID3D12GraphicsCommandList* commandList, const RenderData& renderData);
    void blurSSAO(ID3D12GraphicsCommandList* commandList);
};