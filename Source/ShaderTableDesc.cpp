#include "Globals.h"

#include "ShaderTableDesc.h"

#include "Application.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleD3D12.h"


ShaderTableDesc& ShaderTableDesc::operator=(const ShaderTableDesc&other)
{
    if (this != &other)
    {
        release();
        handle = other.handle;
        refCount = other.refCount;
        addRef();
    }

    return *this;
}

ShaderTableDesc& ShaderTableDesc::operator=(ShaderTableDesc&& other)
{
    if (this != &other)
    {
        release();
        handle = other.handle;
        refCount = other.refCount;
        other.handle = 0;
        other.refCount = nullptr;
    }

    return *this;
}

ShaderTableDesc::operator bool() const 
{ 
    return app->getShaderDescriptors()->isValid(handle); 
}

void ShaderTableDesc::release()
{
    if (refCount && --(*refCount) == 0)
    {
        app->getShaderDescriptors()->deferRelease(handle);

        handle = 0;
        refCount = nullptr;
    }
}

void ShaderTableDesc::createCBV(ID3D12Resource *resource, UINT8 slot)
{
    _ASSERTE(slot < ModuleShaderDescriptors::DESCRIPTORS_PER_TABLE);

    D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
    if (resource)
    {
        D3D12_RESOURCE_DESC desc = resource->GetDesc();
        assert(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);

        viewDesc.BufferLocation = resource->GetGPUVirtualAddress();
        viewDesc.SizeInBytes = UINT(desc.Width);
    }

    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();

    _ASSERTE(descriptors->isValid(handle));

    app->getD3D12()->getDevice()->CreateConstantBufferView(&viewDesc, descriptors->getCPUHandle(handle, slot));
}

void ShaderTableDesc::createTextureSRV(ID3D12Resource *resource, UINT8 slot)
{
    _ASSERTE(slot < ModuleShaderDescriptors::DESCRIPTORS_PER_TABLE);

    if (resource)
    {
        ModuleShaderDescriptors *descriptors = app->getShaderDescriptors();
        _ASSERTE(descriptors->isValid(handle));

        app->getD3D12()->getDevice()->CreateShaderResourceView(resource, nullptr, descriptors->getCPUHandle(handle, slot));
    }
}

void ShaderTableDesc::createTexture2DSRV(ID3D12Resource *resource, UINT arraySlice, UINT mipSlice, UINT8 slot)
{
    if (resource)
    {
        ModuleShaderDescriptors *descriptors = app->getShaderDescriptors();
        _ASSERTE(descriptors->isValid(handle));

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

        app->getD3D12()->getDevice()->CreateShaderResourceView(resource, &viewDesc, descriptors->getCPUHandle(handle, slot));
    }
}

void ShaderTableDesc::createTexture2DUAV(ID3D12Resource *resource, UINT arraySlice, UINT mipSlice, UINT8 slot)
{
    if (resource)
    {
        ModuleShaderDescriptors *descriptors = app->getShaderDescriptors();
        _ASSERTE(descriptors->isValid(handle));

        D3D12_RESOURCE_DESC desc = resource->GetDesc();

        D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc;
        viewDesc.Format = desc.Format;
        viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        viewDesc.Texture2DArray.MipSlice = mipSlice;
        viewDesc.Texture2DArray.FirstArraySlice = arraySlice;
        viewDesc.Texture2DArray.ArraySize = 1;
        viewDesc.Texture2DArray.PlaneSlice = 0;

        app->getD3D12()->getDevice()->CreateUnorderedAccessView(resource, nullptr, &viewDesc, descriptors->getCPUHandle(handle, slot));
    }
}

void ShaderTableDesc::createCubeTextureSRV(ID3D12Resource *resource, UINT8 slot)
{
    _ASSERTE(slot < ModuleShaderDescriptors::DESCRIPTORS_PER_TABLE);

    if (resource)
    {
        ModuleShaderDescriptors *descriptors = app->getShaderDescriptors();
        _ASSERTE(descriptors->isValid(handle));

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = resource->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.MipLevels = -1;
        srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

        app->getD3D12()->getDevice()->CreateShaderResourceView(resource, &srvDesc, descriptors->getCPUHandle(handle, slot));
    }
}

void ShaderTableDesc::createNullTexture2DSRV(UINT8 slot)
{
    _ASSERTE(slot < ModuleShaderDescriptors::DESCRIPTORS_PER_TABLE);

    if (handle != 0)
    {
        ModuleShaderDescriptors *descriptors = app->getShaderDescriptors();
        _ASSERTE(descriptors->isValid(handle));

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Standard format
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

        app->getD3D12()->getDevice()->CreateShaderResourceView(nullptr, &srvDesc, descriptors->getCPUHandle(handle, slot));
    }
}

void ShaderTableDesc::addRef()
{
    if (refCount) ++(*refCount);
}

D3D12_GPU_DESCRIPTOR_HANDLE ShaderTableDesc::getGPUHandle(UINT8 slot) const
{
    return app->getShaderDescriptors()->getGPUHandle(handle, slot);
}

D3D12_CPU_DESCRIPTOR_HANDLE ShaderTableDesc::getCPUHandle(UINT8 slot) const
{
    return app->getShaderDescriptors()->getCPUHandle(handle, slot);
}