#include "Globals.h"

#include "ModuleDynamicBuffer.h"

#include "Application.h"
#include "ModuleD3D12.h"

// 64 MB total buffer size per frame, which should be enough for most use cases. Adjust as needed.
#define BUFFER_TOTAL_SIZE UINT(64 * (1 << 20))

ModuleDynamicBuffer::ModuleDynamicBuffer()
{
}

ModuleDynamicBuffer::~ModuleDynamicBuffer()
{
}

bool ModuleDynamicBuffer::init()
{
    auto* device = app->getD3D12()->getDevice();

    CD3DX12_HEAP_PROPERTIES defaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_HEAP_PROPERTIES stagingProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    for (UINT i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        CD3DX12_RESOURCE_DESC desc  = CD3DX12_RESOURCE_DESC::Buffer(BUFFER_TOTAL_SIZE);
        device->CreateCommittedResource(&defaultProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resources[i].defaultBuffer));
        resources[i].defaultBuffer->SetName(L"Default Ring Buffer");

        device->CreateCommittedResource(&stagingProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resources[i].stagingBuffer));
        resources[i].stagingBuffer->SetName(L"Staging Default Ring Buffer");    

        CD3DX12_RANGE readRange(0, 0);
        resources[i].stagingBuffer->Map(0, &readRange, reinterpret_cast<void**>(&resources[i].mappedPtr));
    }

    return true;
}

void ModuleDynamicBuffer::preRender()
{
    currentFrame = app->getD3D12()->getCurrentBackBufferIdx();
    resources[currentFrame].currentOffset = 0;
    resources[currentFrame].copiedOffset = 0;
}

D3D12_GPU_VIRTUAL_ADDRESS ModuleDynamicBuffer::allocRaw(const void *data, size_t size)
{
    auto& res = resources[currentFrame];

    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = res.defaultBuffer->GetGPUVirtualAddress() + res.currentOffset;

    if (size > 0)
    {
        size_t alignedSize = alignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        if (res.currentOffset + alignedSize > BUFFER_TOTAL_SIZE)
        {
            return 0;
        }

        memcpy(static_cast<uint8_t*>(res.mappedPtr) + res.currentOffset, data, size);
        res.currentOffset += UINT(alignedSize);
    }

    return gpuAddress;
}

void ModuleDynamicBuffer::submitCopy(ID3D12GraphicsCommandList *commandList)
{
    auto& res = resources[currentFrame];

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resources[currentFrame].defaultBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->ResourceBarrier(1, &barrier);

    commandList->CopyBufferRegion(res.defaultBuffer.Get(), res.copiedOffset, res.stagingBuffer.Get(), res.copiedOffset, res.currentOffset - res.copiedOffset);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(res.defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
    commandList->ResourceBarrier(1, &barrier);

    res.copiedOffset = res.currentOffset;
}

