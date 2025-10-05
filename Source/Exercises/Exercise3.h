#pragma once

#include "Module.h"
#include "DebugDrawPass.h"

class Exercise3 : public Module
{
    ComPtr<ID3D12Resource>          vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW        vertexBufferView;
    ComPtr<ID3D12Resource>          bufferUploadHeap;
    ComPtr<ID3D12RootSignature>     rootSignature;
    ComPtr<ID3D12PipelineState>     pso;
    std::unique_ptr<DebugDrawPass>  debugDrawPass;

    Matrix                      mvp;
public:



    virtual bool init() override;
    virtual void render() override;

private:

    bool createVertexBuffer(void* bufferData, unsigned bufferSize, unsigned stride);
    bool createRootSignature();
    bool createPSO();
};
