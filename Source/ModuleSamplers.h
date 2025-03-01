#pragma once

#include "Module.h"
#include "DescriptorHeaps.h"


class ModuleSamplers : public Module
{
public:
    ModuleSamplers();
    ~ModuleSamplers();

    bool init() override;
    bool cleanUp() override;

    uint32_t getNumDefaultSamplers() const {return defaultSamplers.getCount(); }
    D3D12_GPU_DESCRIPTOR_HANDLE getDefaultGPUHandle() const { return defaultSamplers.getGPU(0); }

    ID3D12DescriptorHeap* getHeap() { return heap.getHeap(); }

private:
    
    StaticDescriptorHeap heap;
    DescriptorGroup      defaultSamplers;
};
