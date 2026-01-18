#include "Globals.h"

#include "ModuleRingBuffer.h"

#include "Application.h"
#include "ModuleD3D12.h"

 // 16 Mb
#define UPLOAD_TOTAL_SIZE UINT(16 * (1 << 20))

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
    allocator.init(uploadMemorySize);

    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadMemorySize);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));
    buffer->SetName(L"Upload Ring Buffer");

    CD3DX12_RANGE readRange(0, 0);
    buffer->Map(0, &readRange, reinterpret_cast<void**>(&data));

    return true;
}

void ModuleRingBuffer::preRender()
{
    UINT index = app->getD3D12()->getCurrentBackBufferIdx();
    allocator.updateFrame(index);
}
 

D3D12_GPU_VIRTUAL_ADDRESS ModuleRingBuffer::allocRaw(const void *data, size_t size)
{
    size_t offset = allocator.alloc(alignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
    memcpy(this->data + offset, data, size);
    return buffer->GetGPUVirtualAddress() + offset;
}
