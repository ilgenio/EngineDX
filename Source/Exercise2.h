#pragma once

#include "ModuleRender.h"

class Exercise2 : public Module
{
    
    ComPtr<ID3D12Resource>      vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW    vertexBufferView;
    ComPtr<ID3D12Resource>      bufferUploadHeap;
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;
public:

    virtual bool init() override;
    virtual void render() override;

private:

    bool createVertexBuffer();
    bool createRootSignature();
    bool createPSO();
};
