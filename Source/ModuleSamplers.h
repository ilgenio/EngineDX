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

    const DescriptorGroup& getDefaultGroup() const { return defaultSamplers; }

    ID3D12DescriptorHeap* getHeap() { return heap.getHeap(); }

private:
    
    StaticDescriptorHeap heap;
    DescriptorGroup      defaultSamplers;
};
