#pragma once

#include "DepthStencilDesc.h"
#include "RenderTargetDesc.h"
#include "ShaderTableDesc.h"
#include<span>

struct RenderMesh;
struct RenderData;

class ShadowMapPass
{
    struct BlurConstants
    {
        int directionX;
        int directionY;
        int width;
        int height;
        int sigma;
        int padding[3];
    };

    enum RootParamsShadowPass
    {
        ROOT_SHADOW_MODEL,
        ROOT_SHADOW_VP,

        NUM_ROOT_PARAMETERS_SHADOW 
    };

    enum RootParamsBlur
    {
        ROOT_BLUR_CONSTANTS = 0,
        ROOT_BLUR_INPUT,
        ROOT_BLUR_OUTPUT,
        ROOT_BLUR_SAMPLERS,

        NUM_ROOT_PARAMETERS_BLUR
    };

    ComPtr<ID3D12Resource> depth;
    ComPtr<ID3D12Resource> moments;

    ComPtr<ID3D12Resource> momentsBlur0;
    ComPtr<ID3D12Resource> momentsBlur1;

    DepthStencilDesc dsvDesc;
    RenderTargetDesc rtvDesc;
    ShaderTableDesc  srvDesc;

    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;
    ComPtr<ID3D12RootSignature> blurRootSignature;
    ComPtr<ID3D12PipelineState> blurPSO;

public:
    ShadowMapPass();
    ~ShadowMapPass();

    ShaderTableDesc getSRVDesc() const { return srvDesc; }  

    void render(ID3D12GraphicsCommandList* commandList, std::span<const RenderMesh> meshes, const RenderData& renderData);

    D3D12_GPU_DESCRIPTOR_HANDLE getMomentsSRV() const { return srvDesc.getGPUHandle(4); }

private:

    void renderShadowMap(ID3D12GraphicsCommandList* commandList, std::span<const RenderMesh> meshes, const RenderData& renderData);
    void blurShadowMap(ID3D12GraphicsCommandList* commandList);

    bool createRootSignature();
    bool createPSO();
    bool createShadowMapResources();

    bool createBlurRootSignature();
    bool createBlurPSO();

};