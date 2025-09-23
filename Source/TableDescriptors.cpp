#include "Globals.h"

#include "TableDescriptors.h"

#include "Application.h"
#include "ModuleD3D12.h"

TableDescriptors::TableDescriptors(ComPtr<ID3D12DescriptorHeap> heap, UINT firstDescriptor, UINT descriptorSize) : heap(heap), descriptorSize(descriptorSize) 
{ 
    gpuStart = heap->GetGPUDescriptorHandleForHeapStart();
    gpuStart.ptr += firstDescriptor * descriptorSize;

    cpuStart = heap->GetCPUDescriptorHandleForHeapStart();
    cpuStart.ptr += firstDescriptor * descriptorSize;
}

TableDescriptors::~TableDescriptors()
{
    handles.forceCollectGarbage();

    _ASSERTE(handles.getSize() == handles.getFreeCount());
}

void TableDescriptors::deferRelease(UINT handle)
{
    if (handle != 0)
    {
        handles.deferRelease(handle, app->getD3D12()->getCurrentFrame());
    }
}

void TableDescriptors::collectGarbage()
{
    handles.collectGarbage(app->getD3D12()->getLastCompletedFrame());
}

void TableDescriptors::createCBV(ID3D12Resource *resource, UINT handle, UINT8 slot)
{
    _ASSERTE(slot < DESCRIPTORS_PER_TABLE);

    D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
    if (resource)
    {
        D3D12_RESOURCE_DESC desc = resource->GetDesc();
        assert(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);

        viewDesc.BufferLocation = resource->GetGPUVirtualAddress();
        viewDesc.SizeInBytes = UINT(desc.Width);
    }

    _ASSERTE(handles.validHandle(handle));
    UINT index = handles.indexFromHandle(handle);

    app->getD3D12()->getDevice()->CreateConstantBufferView(&viewDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, index * DESCRIPTORS_PER_TABLE + slot, descriptorSize)); 
}

void TableDescriptors::createTextureSRV(ID3D12Resource *resource, UINT handle, UINT8 slot)
{
    _ASSERTE(slot < DESCRIPTORS_PER_TABLE);

    if (resource)
    {
        _ASSERTE(handles.validHandle(handle));
        UINT index = handles.indexFromHandle(handle);

        app->getD3D12()->getDevice()->CreateShaderResourceView(resource, nullptr, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, index * DESCRIPTORS_PER_TABLE + slot, descriptorSize));
    }
}

void TableDescriptors::createTexture2DSRV(ID3D12Resource* resource, UINT arraySlice, UINT mipSlice, UINT handle, UINT8 slot)
{
    if (resource)
    {
        _ASSERTE(handles.validHandle(handle));
        UINT index = handles.indexFromHandle(handle);

        D3D12_RESOURCE_DESC desc = resource->GetDesc();

        D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc;
        viewDesc.Format = desc.Format;
        viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        viewDesc.Texture2DArray.MostDetailedMip = mipSlice;
        viewDesc.Texture2DArray.MipLevels = 1;
        viewDesc.Texture2DArray.FirstArraySlice = arraySlice;
        viewDesc.Texture2DArray.ArraySize = 1;
        viewDesc.Texture2DArray.PlaneSlice = 0;
        viewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;

        app->getD3D12()->getDevice()->CreateShaderResourceView(resource, &viewDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, index * DESCRIPTORS_PER_TABLE + slot, descriptorSize));
    }
}

void TableDescriptors::createTexture2DUAV(ID3D12Resource* resource, UINT arraySlice, UINT mipSlice, UINT handle, UINT8 slot)
{
    if (resource)
    {
        _ASSERTE(handles.validHandle(handle));
        UINT index = handles.indexFromHandle(handle);

        D3D12_RESOURCE_DESC desc = resource->GetDesc();

        D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc;
        viewDesc.Format = desc.Format;
        viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        viewDesc.Texture2DArray.MipSlice = mipSlice;
        viewDesc.Texture2DArray.FirstArraySlice = arraySlice;
        viewDesc.Texture2DArray.ArraySize = 1;
        viewDesc.Texture2DArray.PlaneSlice = 0;

        app->getD3D12()->getDevice()->CreateUnorderedAccessView(resource, nullptr, &viewDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, index * DESCRIPTORS_PER_TABLE + slot, descriptorSize));
    }
}

void TableDescriptors::createCubeTextureSRV(ID3D12Resource *resource, UINT handle, UINT8 slot)
{
    _ASSERTE(slot < DESCRIPTORS_PER_TABLE);

    if (resource)
    {
        _ASSERTE(handles.validHandle(handle));
        UINT index = handles.indexFromHandle(handle);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = resource->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.MipLevels = -1;
        srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

        app->getD3D12()->getDevice()->CreateShaderResourceView(resource, &srvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, index * DESCRIPTORS_PER_TABLE + slot, descriptorSize));
    }
}
void TableDescriptors::createNullTexture2DSRV(UINT handle, UINT8 slot)
{
    _ASSERTE(slot < DESCRIPTORS_PER_TABLE);

    if (handle != 0)
    {
        _ASSERTE(handles.validHandle(handle));
        UINT index = handles.indexFromHandle(handle);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Standard format
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        app->getD3D12()->getDevice()->CreateShaderResourceView(nullptr, &srvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, index * DESCRIPTORS_PER_TABLE + slot, descriptorSize));
    }
}
