#pragma once

#include "Module.h"
#include "HandleManager.h"
#include "DepthStencilDesc.h"

//-----------------------------------------------------------------------------
// ModuleDSDescriptors manages a pool of depth-stencil view (DSV) descriptors
// for Direct3D 12 resources. It provides creation, reference counting, and
// handle management for depth-stencil descriptors, allowing efficient reuse
// and safe release of DSVs. This module is essential for managing depth
// resources in the rendering pipeline.
//-----------------------------------------------------------------------------
class ModuleDSDescriptors : public Module
{
    friend class DepthStencilDesc;
public:

    ModuleDSDescriptors();
    ~ModuleDSDescriptors();

    bool init() override;

    DepthStencilDesc create(ID3D12Resource* resource);

private:
    void release(UINT handle);
    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(UINT handle) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, handles.indexFromHandle(handle), descriptorSize); }

    bool isValid(UINT handle) const { return handles.validHandle(handle); }
    UINT indexFromHandle(UINT handle) const { return handles.indexFromHandle(handle); }

private:
    enum { MAX_NUM_DEPTHS = 256 };

    typedef HandleManager<MAX_NUM_DEPTHS> Handles;
   
    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = {0};
    UINT descriptorSize = 0;

    Handles handles;
    std::array<UINT, MAX_NUM_DEPTHS> refCounts = { 0 };
};

