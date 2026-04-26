#include "Globals.h"

#include "ModulePerFrameBuffer.h"

#include "Application.h"
#include "ModuleD3D12.h"

// 64 MB total buffer size per frame, which should be enough for most use cases. Adjust as needed.
#define BUFFER_TOTAL_SIZE UINT(64 * (1 << 20))

ModulePerFrameBuffer::ModulePerFrameBuffer()
{
}

ModulePerFrameBuffer::~ModulePerFrameBuffer()
{

}

bool ModulePerFrameBuffer::init()
{
    auto* device = app->getD3D12()->getDevice();

    CD3DX12_HEAP_PROPERTIES defaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_HEAP_PROPERTIES stagingProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    
    CD3DX12_RESOURCE_DESC desc  = CD3DX12_RESOURCE_DESC::Buffer(BUFFER_TOTAL_SIZE);
    device->CreateCommittedResource(&defaultProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&defaultBuffer));
    defaultBuffer->SetName(L"Default Ring Buffer");

    device->CreateCommittedResource(&stagingProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&stagingBuffer));
    stagingBuffer->SetName(L"Staging Default Ring Buffer");    
        
    CD3DX12_RANGE readRange(0, 0);
    stagingBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedPtr));

    return true;
}

void ModulePerFrameBuffer::preRender()
{
    currentOffset = 0;
    copiedOffset = 0;
}

D3D12_GPU_VIRTUAL_ADDRESS ModulePerFrameBuffer::allocRaw(const void *data, size_t size)
{
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = defaultBuffer->GetGPUVirtualAddress() + currentOffset;

    if (size > 0)
    {
        size_t alignedSize = alignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        if (currentOffset + alignedSize > BUFFER_TOTAL_SIZE)
        {
            return 0;
        }

        memcpy(static_cast<uint8_t*>(mappedPtr) + currentOffset, data, size);
        currentOffset += UINT(alignedSize);
    }

    return gpuAddress;
}

void ModulePerFrameBuffer::submitCopy(ID3D12GraphicsCommandList *commandList)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->ResourceBarrier(1, &barrier);

    commandList->CopyBufferRegion(defaultBuffer.Get(), copiedOffset, stagingBuffer.Get(), copiedOffset, currentOffset-copiedOffset);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
    commandList->ResourceBarrier(1, &barrier);

    copiedOffset = currentOffset;
}

