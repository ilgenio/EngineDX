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
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    count          = 16384;
    current        = 0;
    descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = count;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));

    cpuStart = heap->GetCPUDescriptorHandleForHeapStart();
    gpuStart = heap->GetGPUDescriptorHandleForHeapStart();

    return true;
}

bool ModuleDescriptors::cleanUp()
{
    return true;
}

UINT ModuleDescriptors::createCBV(ID3D12Resource *resource)
{
    _ASSERTE(current < count);

    if(current < count)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
        if (resource)
        {
            D3D12_RESOURCE_DESC desc = resource->GetDesc();
            assert(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);

            viewDesc.BufferLocation = resource->GetGPUVirtualAddress();
            viewDesc.SizeInBytes = UINT(desc.Width);
        }

        UINT index = current++;
        app->getD3D12()->getDevice()->CreateConstantBufferView(&viewDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, index, descriptorSize));

        return index;
    }

    return current;
}

UINT ModuleDescriptors::createTextureSRV(ID3D12Resource* resource) 
{
    _ASSERTE(current < count);

    if(current < count)
    {
        UINT index = current++;
        app->getD3D12()->getDevice()->CreateShaderResourceView(resource, nullptr, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, index, descriptorSize));

        return index;
    }

    return current;
}

UINT ModuleDescriptors::createNullTexture2DSRV()
{
    _ASSERTE(current < count);

    if (current < count)
    {
        UINT index = current++;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Standard format
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(cpuStart, index, descriptorSize);
        app->getD3D12()->getDevice()->CreateShaderResourceView(nullptr, &srvDesc, handle);

        return index;
    }

    return current;
}

