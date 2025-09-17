#pragma once

#include "Module.h"


class ModuleRingBuffer : public Module
{
public:

    ModuleRingBuffer();
    ~ModuleRingBuffer();

    bool init() override;

    void preRender() override;

    D3D12_GPU_VIRTUAL_ADDRESS allocBufferRaw(const void* data, size_t size);

    template<typename T>
    D3D12_GPU_VIRTUAL_ADDRESS allocBuffer(const T* data)
    {
        return allocBufferRaw(data, alignUp(sizeof(T), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
    }

    template<typename T>
    D3D12_GPU_VIRTUAL_ADDRESS allocBuffer(const T* data, size_t count)
    {
        return allocBufferRaw(data, alignUp(sizeof(T)*count, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
    }
 

private:

    char*                  bufferData = nullptr;
    ComPtr<ID3D12Resource> buffer;
    size_t                 totalMemorySize = 0;
    size_t                 head = 0;
    size_t                 tail = 0;
    size_t                 allocatedInFrame[FRAMES_IN_FLIGHT];
    size_t                 totalAllocated = 0;
    unsigned               currentFrame = 0;
};

