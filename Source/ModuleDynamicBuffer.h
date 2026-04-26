#pragma once

#include "Module.h"
#include "Ring.h"

class ModuleDynamicBuffer : public Module
{
    ComPtr<ID3D12Resource> stagingBuffer;   // Upload Heap for CPU writes
    ComPtr<ID3D12Resource> defaultBuffer;   // Default Heap for GPU reads
    void* mappedPtr = nullptr;
    UINT currentOffset = 0;
    UINT copiedOffset = 0;

public:

    ModuleDynamicBuffer();
    ~ModuleDynamicBuffer();

    bool init() override;
    void preRender() override;

    template<typename T>
    D3D12_GPU_VIRTUAL_ADDRESS alloc(const T* data)
    {
        return allocRaw(data, sizeof(T));
    }

    template<typename T>
    D3D12_GPU_VIRTUAL_ADDRESS alloc(const T* data, size_t count)
    {
        return allocRaw(data, sizeof(T)*count);
    }

    void submitCopy(ID3D12GraphicsCommandList* commandList);

private:

    D3D12_GPU_VIRTUAL_ADDRESS allocRaw(const void* data, size_t size);
};