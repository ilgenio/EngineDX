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
    handles.forceReleaseDeferred();

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

void TableDescriptors::createCBV(ID3D12Resource *resource, UINT handle, uint8_t slot)
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

void TableDescriptors::createTextureSRV(ID3D12Resource *resource, UINT handle, uint8_t slot)
{
    _ASSERTE(slot < DESCRIPTORS_PER_TABLE);

    if (resource)
    {
        _ASSERTE(handles.validHandle(handle));
        UINT index = handles.indexFromHandle(handle);

        app->getD3D12()->getDevice()->CreateShaderResourceView(resource, nullptr, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, index * DESCRIPTORS_PER_TABLE + slot, descriptorSize));
    }
}

void TableDescriptors::createCubeTextureSRV(ID3D12Resource *resource, UINT handle, uint8_t slot)
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
void TableDescriptors::createNullTexture2DSRV(UINT handle, uint8_t slot)
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
