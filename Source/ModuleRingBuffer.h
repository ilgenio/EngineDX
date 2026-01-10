#pragma once

#include "Module.h"
#include "Ring.h"

// ModuleRingBuffer implements a dynamic ring buffer for fast, frame-based GPU constant buffer allocations in DirectX 12.
// It manages a large upload heap and provides template allocation functions for type-safe buffer uploads.
// The buffer is reset each frame to maximize memory reuse and minimize allocation overhead.
class ModuleRingBuffer : public Module
{
    char*                  uploadData = nullptr;
    ComPtr<ID3D12Resource> uploadBuffer;
    ComPtr<ID3D12Resource> defaultBuffer;
    Ring                   uploadAllocator;
    Ring                   defaultAllocator;
public:

    ModuleRingBuffer();
    ~ModuleRingBuffer();

    bool init() override;
    void preRender() override;

    template<typename T>
    D3D12_GPU_VIRTUAL_ADDRESS allocUploadBuffer(const T* data)
    {
        return allocUploadBufferRaw(data, alignUp(sizeof(T), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
    }

    template<typename T>
    D3D12_GPU_VIRTUAL_ADDRESS allocUploadBuffer(const T* data, size_t count)
    {
        return allocUploadBufferRaw(data, alignUp(sizeof(T)*count, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
    }

    D3D12_GPU_VIRTUAL_ADDRESS allocDefaultBuffer(size_t size)
    {
        return defaultBuffer->GetGPUVirtualAddress() + defaultAllocator.alloc(size);
    }

private:

    D3D12_GPU_VIRTUAL_ADDRESS allocUploadBufferRaw(const void* data, size_t size);

};

