#pragma once

#include "Module.h"
#include "HandleManager.h"

class ModuleShaderDescriptors : public Module
{
public:

    ModuleShaderDescriptors();
    ~ModuleShaderDescriptors();

    bool init() override;

    UINT alloc() { return handles.allocHandle(); }
    void release(UINT handle) { if (handle) handles.freeHandle(handle); }

    UINT createCBV(ID3D12Resource* resource);
    UINT createTextureSRV(ID3D12Resource* resource); 
    UINT createCubeTextureSRV(ID3D12Resource* resource);
    UINT createNullTexture2DSRV();
    
    D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(UINT handle) const { return CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuStart, handles.indexFromHandle(handle), descriptorSize); }
    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(UINT handle) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, handles.indexFromHandle(handle), descriptorSize); }

    ID3D12DescriptorHeap* getHeap() { return heap.Get(); }
private:
    enum { MAX_NUM_DESCRIPTORS = 16384 };

    typedef HandleManager<MAX_NUM_DESCRIPTORS> Handles;
    
    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = {0};
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = {0};
    UINT descriptorSize = 0;

    Handles handles;
};

