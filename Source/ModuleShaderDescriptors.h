#pragma once

#include "Module.h"
#include "HandleManager.h"

class ModuleShaderDescriptors : public Module
{
public:

    ModuleShaderDescriptors();
    ~ModuleShaderDescriptors();

    bool init() override;
    bool cleanUp() override;

    UINT alloc();
    void release(UINT handle);

    UINT createCBV(ID3D12Resource* resource);
    UINT createTextureSRV(ID3D12Resource* resource); 
    UINT createNullTexture2DSRV();

    D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(UINT index) const { return CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuStart, index, descriptorSize); }
    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(UINT index) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, index, descriptorSize); }

    ID3D12DescriptorHeap* getHeap() { return heap.Get(); }
private:
    
    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = {0};
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = {0};
    UINT descriptorSize = 0;
    UINT count = 0;
    UINT current = 0;
};

