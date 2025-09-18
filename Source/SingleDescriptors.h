#pragma once

#include "DeferredFreeHandleManager.h"

class SingleDescriptors 
{
public:
    enum { NUM_DESCRIPTORS = 16384 };
public:
    SingleDescriptors(ComPtr<ID3D12DescriptorHeap> heap, UINT descSize);
    ~SingleDescriptors();

    UINT alloc() { return handles.allocHandle(); }
    void release(UINT handle) { if (handle) handles.freeHandle(handle); }
    void deferRelease(UINT handle);

    void collectGarbage();
    bool isValid(UINT handle) const { return handles.validHandle(handle); }

    UINT createCBV(ID3D12Resource* resource);
    UINT createTextureSRV(ID3D12Resource* resource);
    UINT createCubeTextureSRV(ID3D12Resource* resource);
    UINT createNullTexture2DSRV();

    D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(UINT handle) const { return CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuStart, handles.indexFromHandle(handle), descriptorSize); }
    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(UINT handle) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, handles.indexFromHandle(handle), descriptorSize); }

private:

    typedef DeferredFreeHandleManager<NUM_DESCRIPTORS> Handles;

    Handles handles;
    ComPtr<ID3D12DescriptorHeap> heap;

    D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = {0};
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = {0};
    UINT descriptorSize = 0;
};
