#include "Globals.h"

#include "ModuleDescriptors.h"

#include "Application.h"
#include "ModuleD3D12.h"

ModuleDescriptors::ModuleDescriptors()
{
}

ModuleDescriptors::~ModuleDescriptors()
{
}

bool ModuleDescriptors::init()
{
    const uint32_t numDesriptors[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = { 16384, 512, 512, 128 };

    for(uint32_t i=0; i< D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        heaps[i].init(D3D12_DESCRIPTOR_HEAP_TYPE(i), numDesriptors[i]);
    }

    return true;
}

bool ModuleDescriptors::cleanUp()
{
    return true;
}

bool ModuleDescriptors::allocate(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count, DescriptorGroup& descriptor)
{
    return heaps[type].allocate(count, descriptor);
}

void ModuleDescriptors::writeView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& viewDesc, const DescriptorGroup& descriptor, uint32_t index)
{
    app->getD3D12()->getDevice()->CreateConstantBufferView(&viewDesc, descriptor.getCPU(index));
}

void ModuleDescriptors::writeView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& viewDesc, const DescriptorGroup& descriptor, uint32_t index)
{
    app->getD3D12()->getDevice()->CreateShaderResourceView(resource, &viewDesc, descriptor.getCPU(index));
}

void ModuleDescriptors::writeView(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& viewDesc, const DescriptorGroup& descriptor, uint32_t index)
{
    app->getD3D12()->getDevice()->CreateUnorderedAccessView(resource, nullptr, &viewDesc, descriptor.getCPU(index));
}

void ModuleDescriptors::writeView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& viewDesc, const DescriptorGroup& descriptor, uint32_t index)
{
    app->getD3D12()->getDevice()->CreateRenderTargetView(resource, &viewDesc, descriptor.getCPU(index));
}

void ModuleDescriptors::writeView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& viewDesc, const DescriptorGroup& descriptor, uint32_t index)
{
    app->getD3D12()->getDevice()->CreateDepthStencilView(resource, &viewDesc, descriptor.getCPU(index));
}

void ModuleDescriptors::writeSampler(D3D12_SAMPLER_DESC& samplerDesc, const DescriptorGroup& descriptor, uint32_t index)
{
    app->getD3D12()->getDevice()->CreateSampler(&samplerDesc, descriptor.getCPU(index));
}

