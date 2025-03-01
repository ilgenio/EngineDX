#pragma once

#include "ModuleRender.h"
#include "DescriptorHeaps.h"

#include <vector>

namespace DirectX
{
    class ScratchImage;
}

class Exercise4 : public Module
{
    ComPtr<ID3D12Fence1>         uploadFence;
    HANDLE                       uploadEvent = NULL;
    unsigned                     uploadFenceCounter = 0;

    ComPtr<ID3D12Resource>       textureDog;
    DescriptorGroup              srvDog;
    ComPtr<ID3D12Resource>       vertexBuffer;
    ComPtr<ID3D12Resource>       indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW     vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW      indexBufferView;
    ComPtr<ID3D12RootSignature>  rootSignature;
    ComPtr<ID3D12PipelineState>  pso;
    ComPtr<ID3DBlob>             vertexShader;
    ComPtr<ID3DBlob>             pixelShader;

    Matrix mvp;

public:

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void render() override;

private:

    bool createVertexBuffer(void* bufferData, unsigned bufferSize, unsigned stride);
    bool createIndexBuffer(void* bufferData, unsigned bufferSize);
    bool createRootSignature();
    bool createPSO();
    bool createUploadFence();
};
