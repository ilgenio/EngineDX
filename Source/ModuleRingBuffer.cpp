#include "Globals.h"

#include "ModuleRingBuffer.h"

#include "Application.h"
#include "ModuleD3D12.h"

 // 10 Mb
#define MEMORY_TOTAL_SIZE UINT(10 * (1 << 20))

ModuleRingBuffer::ModuleRingBuffer()
{
}

ModuleRingBuffer::~ModuleRingBuffer()
{
}

bool ModuleRingBuffer::init()
{
    totalMemorySize = alignUp(MEMORY_TOTAL_SIZE, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();
    ID3D12CommandQueue* queue = d3d12->getDrawCommandQueue();

    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(totalMemorySize);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));
    buffer->SetName(L"Dynamic Ring Buffer");

    CD3DX12_RANGE readRange(0, 0);
    buffer->Map(0, &readRange, reinterpret_cast<void**>(&bufferData));

    head = 0;
    tail = 0;
    currentFrame = d3d12->getCurrentBackBufferIdx();
    totalAllocated = 0;

    for(int i=0; i< FRAMES_IN_FLIGHT; ++i)
    {
        allocatedInFrame[i] = 0;
    }

    return true;
}


void ModuleRingBuffer::preRender() 
{
    ModuleD3D12* d3d12 = app->getD3D12();

    currentFrame = d3d12->getCurrentBackBufferIdx();

    tail = (tail+allocatedInFrame[currentFrame])%totalMemorySize;
    totalAllocated -= allocatedInFrame[currentFrame];

    allocatedInFrame[currentFrame] = 0;
}

D3D12_GPU_VIRTUAL_ADDRESS ModuleRingBuffer::allocBufferRaw(const void* data, size_t size)
{
    _ASSERT_EXPR(size < (totalMemorySize-totalAllocated), L"Out of memory, please allocate more memory at initialisation");
    _ASSERT_EXPR((size & (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT -1) ) == 0, "Size must be multiple of D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT"); // Is aligned with 256u 

    if(tail < head)
    {
        size_t availableToEnd = size_t(totalMemorySize-head);
        if(availableToEnd >= size)
        {
            memcpy(bufferData+head, data, size);

            D3D12_GPU_VIRTUAL_ADDRESS address = buffer->GetGPUVirtualAddress() + head;
            head += size;

            allocatedInFrame[currentFrame] += size;
            totalAllocated += size;

            return address; 

        }
        else
        {
            // Force contiguous memory, allocate to end and try again with tail >= head
            head = 0;
            allocatedInFrame[currentFrame] += availableToEnd;
            totalAllocated += availableToEnd;
        }
    }

    _ASSERTE(head <= tail);
    _ASSERT_EXPR(size < (totalMemorySize - totalAllocated), L"Out of memory, please allocate more memory at initialisation");

    size_t available = tail == head && totalAllocated == 0 ? totalMemorySize : size_t(tail-head);

    if(size < available)
    {
        memcpy(bufferData+head, data, size);

        D3D12_GPU_VIRTUAL_ADDRESS address = buffer->GetGPUVirtualAddress() + head;
        head += size;

        allocatedInFrame[currentFrame] += size;
        totalAllocated += size;

        return address;
    }

    return 0;
}
