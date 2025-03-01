#pragma once

#include "Module.h"
#include "DescriptorHeaps.h"

class ModuleDescriptors : public Module
{
public:

    ModuleDescriptors();
    ~ModuleDescriptors();

    bool init() override;
    bool cleanUp() override;

    bool allocateDescGroup(uint32_t count, DescriptorGroup& descGroup);

    void createCBV(ID3D12Resource* resource, const DescriptorGroup& descriptor, uint32_t index = 0);
    void createTextureSRV(ID3D12Resource* resource, const DescriptorGroup& descriptor, uint32_t index = 0); 

    ID3D12DescriptorHeap* getHeap() { return heap.getHeap(); }

private:
    
    StaticDescriptorHeap heap;
};

