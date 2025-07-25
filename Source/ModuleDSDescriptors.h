#pragma once

#include "Module.h"
#include "HandleManager.h"

class ModuleDSDescriptors : public Module
{
public:

    ModuleDSDescriptors();
    ~ModuleDSDescriptors();

    bool init() override;

    UINT create(ID3D12Resource* resource);
    void release(UINT handle);

    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(UINT handle) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, handles.indexFromHandle(handle), descriptorSize); }

private:
    enum { MAX_NUM_DEPTHS = 256 };

    typedef HandleManager<MAX_NUM_DEPTHS> Handles;
   
    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = {0};
    UINT descriptorSize = 0;

    Handles handles;
};

