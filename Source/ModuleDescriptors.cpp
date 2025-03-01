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
    heap.init(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16384);

    return true;
}

bool ModuleDescriptors::cleanUp()
{
    return true;
}

bool ModuleDescriptors::allocateDescGroup(uint32_t count, DescriptorGroup& descriptor)
{
    return heap.allocate(count, descriptor);
}

void ModuleDescriptors::createCBV(ID3D12Resource *resource, const DescriptorGroup &descriptor, uint32_t index)
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
    if (resource)
    {
        D3D12_RESOURCE_DESC desc = resource->GetDesc();
        assert(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);

        viewDesc.BufferLocation = resource->GetGPUVirtualAddress();
        viewDesc.SizeInBytes = UINT(desc.Width);
    }

    app->getD3D12()->getDevice()->CreateConstantBufferView(&viewDesc, descriptor.getCPU(index));
}

void ModuleDescriptors::createTextureSRV(ID3D12Resource* resource, const DescriptorGroup& descriptor, uint32_t index) 
{
    app->getD3D12()->getDevice()->CreateShaderResourceView(resource, nullptr, descriptor.getCPU(index));
}

