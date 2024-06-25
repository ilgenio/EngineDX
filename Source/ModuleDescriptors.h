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

    bool allocateDescGroup(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count, DescriptorGroup& descGroup);

    void createCBV(ID3D12Resource* resource, const DescriptorGroup& descriptor, uint32_t index);
    void createTextureSRV(ID3D12Resource* resource, const DescriptorGroup& descriptor, uint32_t index, bool isCubemap = false);
    void createBufferSRV(ID3D12Resource* resource, const DescriptorGroup& descriptor, uint32_t index, uint32_t numStructuredElements, uint32_t structuredStride);

    ID3D12DescriptorHeap* getHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) { return heaps[type].getHeap(); }

private:
    
    StaticDescriptorHeap heaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
};

