#pragma once

#include "Module.h"
#include "HandleManager.h"

class ModuleRTDescriptors : public Module
{
public:

    ModuleRTDescriptors();
    ~ModuleRTDescriptors();

    bool init() override;

    UINT create(ID3D12Resource* resource);
    void release(UINT handle);

    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(UINT handle) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, handles.indexFromHandle(handle), descriptorSize); }

private:
    enum { MAX_NUM_TARGETS = 256 };

    typedef HandleManager<MAX_NUM_TARGETS> Handles;
   
    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = {0};
    UINT descriptorSize = 0;

    Handles handles;
};

