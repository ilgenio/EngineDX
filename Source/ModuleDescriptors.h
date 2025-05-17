#pragma once

#include "Module.h"

class ModuleDescriptors : public Module
{
public:

    ModuleDescriptors();
    ~ModuleDescriptors();

    bool init() override;
    bool cleanUp() override;

    UINT createCBV(ID3D12Resource* resource);
    UINT createTextureSRV(ID3D12Resource* resource); 
    UINT createNullTexture2DSRV();

    UINT allocateDescriptors(UINT numDescriptors = 1)
    {
        _ASSERTE(current + numDescriptors <= count);
        UINT index = current;
        current += numDescriptors;
        return index;         
    }

    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(UINT index) const
    {
        _ASSERTE(index < current);
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, index, descriptorSize);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(UINT index) const
    {
        _ASSERTE(index < current);
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuStart, index, descriptorSize);
    }

    ID3D12DescriptorHeap* getHeap() { return heap.Get(); }

private:
    
    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = {0};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = {0};
    UINT descriptorSize = 0;
    UINT count = 0;
    UINT current = 0;
};

