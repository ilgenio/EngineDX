#pragma once

#include "ModuleRender.h"
#include "DescriptorHeaps.h"
#include "DebugDrawPass.h"
#include "ImGuiPass.h"

#include <vector>

namespace DirectX
{
    class ScratchImage;
}

class Exercise4 : public Module
{
    enum ESamplers
    {
        SAMPLER_LINEAR_WRAP = 0,
        SAMPLER_POINT_WRAP,
        SAMPLER_LINEAR_CLAMP,
        SAMPLER_POINT_CLAMP,
        SAMPLER_COUNT
    };

    ComPtr<ID3D12Fence1>            uploadFence;
    HANDLE                          uploadEvent = NULL;
    unsigned                        uploadFenceCounter = 0;

    ComPtr<ID3D12Resource>          textureDog;
    DescriptorGroup                 srvDog;
    ComPtr<ID3D12Resource>          vertexBuffer;
    ComPtr<ID3D12Resource>          indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW        vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW         indexBufferView;
    ComPtr<ID3D12RootSignature>     rootSignature;
    ComPtr<ID3D12PipelineState>     pso;
    ComPtr<ID3DBlob>                vertexShader;
    ComPtr<ID3DBlob>                pixelShader;
    std::unique_ptr<DebugDrawPass>  debugDrawPass;
    std::unique_ptr<ImGuiPass>      imguiPass;
    DescriptorGroup                 debugFonts;
    bool                            showAxis = true;
    bool                            showGrid = true;
    int                             sampler = int(SAMPLER_LINEAR_WRAP);

    Matrix mvp;

public:

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void preRender() override;
    virtual void render() override;

private:

    bool createVertexBuffer(void* bufferData, unsigned bufferSize, unsigned stride);
    bool createIndexBuffer(void* bufferData, unsigned bufferSize);
    bool createRootSignature();
    bool createPSO();
    bool createUploadFence();
};
