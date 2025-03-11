#pragma once

#include "Module.h"


class ModuleSamplers : public Module
{
public:
    enum Type
    {
        LINEAR_WRAP,
        POINT_WRAP,
        LINEAR_CLAMP,
        POINT_CLAMP,
        COUNT
    };
public:
    
    ModuleSamplers();
    ~ModuleSamplers();

    bool init() override;
    bool cleanUp() override;

    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHanlde(Type type) const
    {
        _ASSERTE(type < COUNT);

        return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, type, descriptorSize);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE getGPUHanlde(Type type) const
    {
        _ASSERTE(type < COUNT);

        return CD3DX12_GPU_DESCRIPTOR_HANDLE(gpuStart, type, descriptorSize);
    }


    ID3D12DescriptorHeap* getHeap() { return heap.Get(); }

private:
    
    ComPtr<ID3D12DescriptorHeap> heap;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = { 0 };
    UINT descriptorSize = 0;
    UINT count = 0;
};
