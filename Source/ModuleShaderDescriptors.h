#pragma once

#include "Module.h"

#include "DeferredFreeHandleManager.h"
#include "ShaderTableDesc.h"

class ModuleShaderDescriptors : public Module
{
    friend class ShaderTableDesc;

public:

    ModuleShaderDescriptors();
    ~ModuleShaderDescriptors();

    bool init() override;
    void preRender() override;

    ID3D12DescriptorHeap* getHeap() { return heap.Get(); }

    ShaderTableDesc allocTable();

private:
    
    UINT alloc() { return handles.allocHandle(); }
    void release(UINT handle) { if (handle) handles.freeHandle(handle); }
    void deferRelease(UINT handle);
    void collectGarbage();

    bool isValid(UINT handle) const { return handles.validHandle(handle); }
    UINT indexFromHandle(UINT handle) const { return handles.indexFromHandle(handle); }

    D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(UINT handle, UINT8 slot) const { return CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuStart, handles.indexFromHandle(handle)*DESCRIPTORS_PER_TABLE+slot, descriptorSize); }
    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(UINT handle, UINT8 slot) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, handles.indexFromHandle(handle)*DESCRIPTORS_PER_TABLE+slot, descriptorSize); }

private:

    enum { NUM_DESCRIPTORS = 4096, DESCRIPTORS_PER_TABLE = 8 };

    typedef DeferredFreeHandleManager<NUM_DESCRIPTORS> Handles;

    ComPtr<ID3D12DescriptorHeap> heap;

    Handles handles;

    D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = {0};
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = {0};
    UINT descriptorSize = 0;

    std::array<UINT, NUM_DESCRIPTORS> refCounts = {0};
};
