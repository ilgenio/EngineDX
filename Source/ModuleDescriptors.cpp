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

bool ModuleDescriptors::allocateDescGroup(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count, DescriptorGroup& descriptor)
{
    return heaps[type].allocate(count, descriptor);
}

void ModuleDescriptors::createCBV(ID3D12Resource *resource, const DescriptorGroup &descriptor, uint32_t index)
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
    if(resource)
    {
        D3D12_RESOURCE_DESC desc = resource->GetDesc();
        assert(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);

        viewDesc.BufferLocation = resource->GetGPUVirtualAddress();
        viewDesc.SizeInBytes = UINT(desc.Width);
    }

    app->getD3D12()->getDevice()->CreateConstantBufferView(&viewDesc, descriptor.getCPU(index));
}

void ModuleDescriptors::createTextureSRV(ID3D12Resource* resource, const DescriptorGroup& descriptor, uint32_t index, bool isCubemap)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};

    if(resource)
    {
        D3D12_RESOURCE_DESC desc = resource->GetDesc();
        viewDesc.Format = desc.Format;
        viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        switch (desc.Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
            assert(false && "Only textures supported. Call CreateBufferSRV");
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            if (desc.DepthOrArraySize > 1)
            {
                viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                viewDesc.Texture1DArray = {0, desc.MipLevels, 0, desc.DepthOrArraySize, 0.0f};
            }
            else
            {
                viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                viewDesc.Texture1D = {0, desc.MipLevels, 0.0f};
            }
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (isCubemap)
            {
                assert(desc.DepthOrArraySize >= 6 && desc.DepthOrArraySize % 6 == 0);
                if (desc.DepthOrArraySize > 6)
                {
                    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                    viewDesc.TextureCubeArray = {0, desc.MipLevels, 0, UINT(desc.DepthOrArraySize / 6), 0.0f};
                }
                else
                {
                    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                    viewDesc.TextureCube = {0, desc.MipLevels, 0.0f};
                }
            }
            else
            {
                if (desc.DepthOrArraySize > 1)
                {
                    if (desc.SampleDesc.Count > 1)
                    {
                        viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                        viewDesc.Texture2DMSArray = {0, desc.DepthOrArraySize};
                    }
                    else
                    {
                        viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                        viewDesc.Texture2DArray = {0, desc.MipLevels, 0, desc.DepthOrArraySize, 0, 0.0f};
                    }
                }
                else
                {
                    if (desc.SampleDesc.Count > 1)
                    {
                        viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
                    }
                    else
                    {
                        viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                        viewDesc.Texture2D = {0, desc.MipLevels, 0, 0.0f};
                    }
                }
            }
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
            viewDesc.Texture3D = {0, desc.MipLevels, 0.0f};
            break;
        }
    }

    app->getD3D12()->getDevice()->CreateShaderResourceView(resource, &viewDesc, descriptor.getCPU(index));
}

void ModuleDescriptors::createBufferSRV(ID3D12Resource* resource, const DescriptorGroup& descriptor, uint32_t index, uint32_t numStructuredElements, uint32_t structuredStride)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};

    viewDesc.Format = DXGI_FORMAT_UNKNOWN;
    viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    assert(resource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);

    viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    viewDesc.Buffer = { 0, UINT(numStructuredElements), UINT(structuredStride), D3D12_BUFFER_SRV_FLAG_NONE};

    app->getD3D12()->getDevice()->CreateShaderResourceView(resource, &viewDesc, descriptor.getCPU(index));
}
