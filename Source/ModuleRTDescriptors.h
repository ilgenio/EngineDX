#pragma once

#include "Module.h"
#include "DeferredFreeHandleManager.h"

class ModuleRTDescriptors : public Module
{
public:

    ModuleRTDescriptors();
    ~ModuleRTDescriptors();

    bool init() override;
    void preRender() override;

    UINT create(ID3D12Resource* resource);
    UINT create(ID3D12Resource* resource, UINT arraySlice, UINT mipSlice, DXGI_FORMAT format);
    void release(UINT handle);
    void deferRelease(UINT handle);

    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(UINT handle) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, handles.indexFromHandle(handle), descriptorSize); }

private:
    enum { MAX_NUM_TARGETS = 256 };

    typedef DeferredFreeHandleManager<MAX_NUM_TARGETS> Handles;
   
    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = {0};
    UINT descriptorSize = 0;

    Handles handles;

    UINT deferredFreeCount = 0;
};

