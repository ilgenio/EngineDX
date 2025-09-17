#include "Globals.h"
#include "SingleDescriptors.h"

#include "Application.h"
#include "ModuleD3D12.h"

SingleDescriptors::SingleDescriptors(ComPtr<ID3D12DescriptorHeap> heap, UINT descSize) : heap(heap), descriptorSize(descSize)
{ 
    gpuStart = heap->GetGPUDescriptorHandleForHeapStart();
    cpuStart = heap->GetCPUDescriptorHandleForHeapStart();
}

SingleDescriptors::~SingleDescriptors()
{
    handles.forceReleaseDeferred();

    _ASSERTE(handles.getSize() == handles.getFreeCount());
}

void SingleDescriptors::deferRelease(UINT handle)
{
    if (handle != 0)
    {
        handles.deferRelease(handle, app->getD3D12()->getCurrentFrame());
    }
}

void SingleDescriptors::releaseDeferred()
{
    handles.releaseDeferred(app->getD3D12()->getLastCompletedFrame());
}

UINT SingleDescriptors::createCBV(ID3D12Resource *resource)
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

UINT SingleDescriptors::createTextureSRV(ID3D12Resource* resource) 
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

UINT SingleDescriptors::createCubeTextureSRV(ID3D12Resource* resource)
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

UINT SingleDescriptors::createNullTexture2DSRV()
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
