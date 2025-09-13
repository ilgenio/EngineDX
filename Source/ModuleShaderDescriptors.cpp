#include "Globals.h"

#include "ModuleShaderDescriptors.h"

#include "Application.h"
#include "ModuleD3D12.h"

ModuleShaderDescriptors::ModuleShaderDescriptors()
{
}

ModuleShaderDescriptors::~ModuleShaderDescriptors()
{
    handles.forceReleaseDeferred();

    _ASSERTE((handles.getSize() - handles.getFreeCount()) == 0);
}

bool ModuleShaderDescriptors::init()
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = MAX_NUM_DESCRIPTORS;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));

    heap->SetName(L"Module Descriptors Heap");

    gpuStart = heap->GetGPUDescriptorHandleForHeapStart();
    cpuStart = heap->GetCPUDescriptorHandleForHeapStart();

    return true;
}

void ModuleShaderDescriptors::preRender()
{
    handles.releaseDeferred(app->getD3D12()->getLastCompletedFrame());
}

void ModuleShaderDescriptors::deferRelease(UINT handle)
{
    if (handle != 0)
    {
        handles.deferRelease(handle, app->getD3D12()->getCurrentFrame());
    }
}

UINT ModuleShaderDescriptors::createCBV(ID3D12Resource *resource)
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
    if (resource)
    {
        D3D12_RESOURCE_DESC desc = resource->GetDesc();
        assert(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);

        viewDesc.BufferLocation = resource->GetGPUVirtualAddress();
        viewDesc.SizeInBytes = UINT(desc.Width);
    }

    UINT handle = handles.allocHandle();
    _ASSERTE(handles.validHandle(handle));

    app->getD3D12()->getDevice()->CreateConstantBufferView(&viewDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, handles.indexFromHandle(handle), descriptorSize));

    return handle;
}

UINT ModuleShaderDescriptors::createTextureSRV(ID3D12Resource* resource) 
{
    if(resource)
    {
        UINT handle = handles.allocHandle();
        _ASSERTE(handles.validHandle(handle));

        app->getD3D12()->getDevice()->CreateShaderResourceView(resource, nullptr, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, handles.indexFromHandle(handle), descriptorSize));

        return handle;
    }

    return 0;
}

UINT ModuleShaderDescriptors::createCubeTextureSRV(ID3D12Resource* resource)
{
    if (resource)
    {
        UINT handle = handles.allocHandle();
        _ASSERTE(handles.validHandle(handle));

        D3D12_RESOURCE_DESC desc = resource->GetDesc();

        D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc;
        viewDesc.Format = desc.Format;
        viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        viewDesc.TextureCube.MipLevels = desc.MipLevels;
        viewDesc.TextureCube.MostDetailedMip = 0;
        viewDesc.Texture1D.ResourceMinLODClamp = 0.0f;

        app->getD3D12()->getDevice()->CreateShaderResourceView(resource, &viewDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, handles.indexFromHandle(handle), descriptorSize));

        return handle;
    }

    return 0;
}

UINT ModuleShaderDescriptors::createNullTexture2DSRV()
{
    UINT handle = handles.allocHandle();
    _ASSERTE(handles.validHandle(handle));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Standard format
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    
    app->getD3D12()->getDevice()->CreateShaderResourceView(nullptr, &srvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, handles.indexFromHandle(handle), descriptorSize));

    return handle;
}

