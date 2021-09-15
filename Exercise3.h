#pragma once

#include "ModuleRender.h"

namespace DirectX
{
    class ScratchImage;
}

class Exercise3 : public Module
{
    ComPtr<ID3D12Resource>       texture;
    ComPtr<ID3D12Resource>       vertexBuffer;
    ComPtr<ID3D12Resource>       indexBuffer;
    D3D12_VERTEX_BUFFER_VIEW     vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW      indexBufferView;
    ComPtr<ID3D12Resource>       vBufferUploadHeap;
    ComPtr<ID3D12Resource>       iBufferUploadHeap;
    ComPtr<ID3D12Resource>       textureUploadHeap;
    ComPtr<ID3D12DescriptorHeap> mainDescriptorHeap;
    ComPtr<ID3D12RootSignature>  rootSignature;
    ComPtr<ID3D12PipelineState>  pso;
    ComPtr<ID3DBlob>             vertexShader;
    ComPtr<ID3DBlob>             pixelShader;

    XMFLOAT4X4                   mvp;
public:

    virtual bool init() override;
    virtual UpdateStatus update() override;

private:

    bool createVertexBuffer(void* bufferData, unsigned bufferSize, unsigned stride);
    bool createIndexBuffer(void* bufferData, unsigned bufferSize);
    bool createBuffer(void* bufferData, unsigned bufferSize, ComPtr<ID3D12Resource>& buffer, ComPtr<ID3D12Resource>& upload);
    bool createShaders();
    bool createMainDescriptorHeap();
    bool createRootSignature();
    bool createPSO();
    bool loadTextureFromFile(const wchar_t* fileName);
    bool loadTexture(const ScratchImage& image);
};