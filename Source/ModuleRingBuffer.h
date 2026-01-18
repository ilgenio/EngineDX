#pragma once

#include "Module.h"
#include "Ring.h"

// ModuleRingBuffer implements a dynamic ring buffer for fast, frame-based GPU constant buffer allocations in DirectX 12.
// It manages a large upload heap and provides template allocation functions for type-safe buffer uploads.
// The buffer is reset each frame to maximize memory reuse and minimize allocation overhead.
class ModuleRingBuffer : public Module
{
    UINT8*                  data = nullptr;
    ComPtr<ID3D12Resource> buffer;
    Ring                   allocator;
public:

    ModuleRingBuffer();
    ~ModuleRingBuffer();

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


private:

    D3D12_GPU_VIRTUAL_ADDRESS allocRaw(const void* data, size_t size);

};

