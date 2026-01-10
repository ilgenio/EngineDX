#include "Globals.h"

#include "ModuleRingBuffer.h"

#include "Application.h"
#include "ModuleD3D12.h"

 // 16 Mb
#define UPLOAD_TOTAL_SIZE UINT(16 * (1 << 20))

 // 128 Mb
#define DEFAULT_TOTAL_SIZE UINT(128 * (1 << 20))

ModuleRingBuffer::ModuleRingBuffer()
{
}

ModuleRingBuffer::~ModuleRingBuffer()
{
}

bool ModuleRingBuffer::init()
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    // Upload
    size_t uploadMemorySize = alignUp(UPLOAD_TOTAL_SIZE, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    uploadAllocator.init(uploadMemorySize);

    CD3DX12_HEAP_PROPERTIES uploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadMemorySize);
    device->CreateCommittedResource(&uploadProps, D3D12_HEAP_FLAG_NONE, &uploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuffer));
    uploadBuffer->SetName(L"Upload Ring Buffer");

    CD3DX12_RANGE readRange(0, 0);
    uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&uploadData));

    // Default
    size_t defaultMemorySize = alignUp(DEFAULT_TOTAL_SIZE, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    defaultAllocator.init(defaultMemorySize);

    CD3DX12_HEAP_PROPERTIES defaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC defaultDesc = CD3DX12_RESOURCE_DESC::Buffer(defaultMemorySize);
    device->CreateCommittedResource(&defaultProps, D3D12_HEAP_FLAG_NONE, &defaultDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&defaultBuffer));
    defaultBuffer->SetName(L"Default Ring Buffer");

    return true;
}

void ModuleRingBuffer::preRender() 
{
    UINT index = app->getD3D12()->getCurrentBackBufferIdx();
    uploadAllocator.updateFrame(index);
    defaultAllocator.updateFrame(index);
}

D3D12_GPU_VIRTUAL_ADDRESS ModuleRingBuffer::allocUploadBufferRaw(const void *data, size_t size)
{
    size_t offset = uploadAllocator.alloc(size);
    memcpy(uploadData + offset, data, size);
    return uploadBuffer->GetGPUVirtualAddress() + offset;
}
