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

    bool allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count, DescriptorGroup& descriptor);

    void writeView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& viewDesc, const DescriptorGroup& descriptor, uint32_t index);
    void writeView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& viewDesc, const DescriptorGroup& descriptor, uint32_t index);
    void writeView(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& viewDesc, const DescriptorGroup& descriptor, uint32_t index);
    void writeView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& viewDesc, const DescriptorGroup& descriptor, uint32_t index);
    void writeView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& viewDesc, const DescriptorGroup& descriptor, uint32_t index);

    void writeSampler(D3D12_SAMPLER_DESC& samplerDesc, const DescriptorGroup& descriptor, uint32_t index);

private:
    
    StaticDescriptorHeap heaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
};

