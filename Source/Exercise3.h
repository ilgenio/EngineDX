#pragma once

#include "ModuleRender.h"
#include <vector>

namespace DirectX
{
    class ScratchImage;
}

class Exercise3 : public Module
{
    ComPtr<ID3D12Fence1>         uploadFence;
    HANDLE                       uploadEvent = NULL;
    unsigned                     uploadFenceCounter = 0;

    ComPtr<ID3D12Resource>       texture;
    ComPtr<ID3D12Resource>       textureDog;
    ComPtr<ID3D12Resource>       vertexBuffer;
    ComPtr<ID3D12Resource>       indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW     vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW      indexBufferView;
    ComPtr<ID3D12DescriptorHeap> mainDescriptorHeap;
    ComPtr<ID3D12RootSignature>  rootSignature;
    ComPtr<ID3D12PipelineState>  pso;
    ComPtr<ID3DBlob>             vertexShader;
    ComPtr<ID3DBlob>             pixelShader;

    std::vector<ComPtr<ID3D12Resource> >    textureUploadHeaps;

    //XMFLOAT4X4                   mvp;
    Matrix mvp;
public:

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void render() override;

private:

    bool createVertexBuffer(void* bufferData, unsigned bufferSize, unsigned stride);
    bool createIndexBuffer(void* bufferData, unsigned bufferSize);
    bool createBuffer(void* bufferData, unsigned bufferSize, ComPtr<ID3D12Resource>& buffer, D3D12_RESOURCE_STATES initialState);
    bool createShaders();
    bool createMainDescriptorHeap();
    bool createRootSignature();
    bool createPSO();
    bool createUploadFence();
    bool loadTextureFromFile(const wchar_t* fileName, ComPtr<ID3D12Resource>& texResource);
    bool loadTexture(const ScratchImage& image, ComPtr<ID3D12Resource>& texResource);
};