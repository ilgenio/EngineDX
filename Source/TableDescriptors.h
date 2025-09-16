#pragma once

#include "DeferredFreeHandleManager.h"

class TableDescriptors
{
public:
    enum { NUM_DESCRIPTORS = 4096, DESCRIPTORS_PER_TABLE = 8 };

public:
    TableDescriptors(ComPtr<ID3D12DescriptorHeap> heap, UINT firstDescriptor, UINT descriptorSize);
    ~TableDescriptors();

    UINT alloc() { return handles.allocHandle(); }
    void release(UINT handle) { if (handle) handles.freeHandle(handle); }
    void deferRelease(UINT handle);

    void releaseDeferred();

    void createCBV(ID3D12Resource* resource, UINT handle, uint8_t slot);
    void createTextureSRV(ID3D12Resource* resource, UINT handle, uint8_t slot);
    void createCubeTextureSRV(ID3D12Resource* resource, UINT handle, uint8_t slot);
    void createNullTexture2DSRV(UINT handle, uint8_t slot);

    bool isValid(UINT handle) const { return handles.validHandle(handle); }

    D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(UINT handle, UINT slot = 0) const { return CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuStart, handles.indexFromHandle(handle)*DESCRIPTORS_PER_TABLE+slot, descriptorSize); }
    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(UINT handle, UINT slot = 0) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, handles.indexFromHandle(handle)*DESCRIPTORS_PER_TABLE+slot, descriptorSize); }

private:

    typedef DeferredFreeHandleManager<NUM_DESCRIPTORS> Handles;

    Handles handles;
    ComPtr<ID3D12DescriptorHeap> heap;

    D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = {0};
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = {0};
    UINT descriptorSize = 0;
};
