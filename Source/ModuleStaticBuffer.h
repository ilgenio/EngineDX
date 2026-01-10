#pragma once

#include "Module.h"

class ModuleStaticBuffer : public Module
{
public:
    explicit ModuleStaticBuffer();
    ~ModuleStaticBuffer() override;

    bool init() override;

    void reset();
    bool allocVertexBuffer(UINT numVertices, UINT strideInBytes, const void* initData, D3D12_VERTEX_BUFFER_VIEW& view);
    bool allocIndexBuffer(UINT numIndices, UINT strideInBytes, const void* initData, D3D12_INDEX_BUFFER_VIEW& view);
    bool allocConstantBuffer(UINT size, const void* initData, D3D12_CONSTANT_BUFFER_VIEW_DESC& viewDesc);
    bool allocBuffer(UINT size, const void* initData, D3D12_GPU_VIRTUAL_ADDRESS& bufferLocation)
    {
        UINT outSize;
        return allocBuffer(size, initData, bufferLocation, outSize);
    }

    void uploadData();

    ComPtr<ID3D12Resource> getResource() { return vidMemBuffer; }

private:

    bool allocBuffer(UINT size, const void* initData, D3D12_GPU_VIRTUAL_ADDRESS& bufferLocation, UINT& outSize);


private:

    BYTE* data = nullptr;
    UINT totalSize = 0;
    UINT offset = 0;
    UINT vidInit = 0;

    ComPtr<ID3D12Resource> uploadHeapBuffer;
    ComPtr<ID3D12Resource> vidMemBuffer;
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;
};