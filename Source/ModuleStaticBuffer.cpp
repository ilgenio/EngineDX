#include "Globals.h"
#include "ModuleStaticBuffer.h"

#include "Application.h"
#include "ModuleD3D12.h"

ModuleStaticBuffer::ModuleStaticBuffer()
{
}

ModuleStaticBuffer::~ModuleStaticBuffer() 
{
}

bool ModuleStaticBuffer::init() 
{
    ModuleD3D12* d3d12 = app->getD3D12();

    totalSize = 512 << 20; // 512 MB - 1/2 GB

    bool ok = SUCCEEDED(d3d12->getDevice()->CreateCommittedResource(
                    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                    D3D12_HEAP_FLAG_NONE,
                    &CD3DX12_RESOURCE_DESC::Buffer(totalSize, D3D12_RESOURCE_FLAG_NONE),
                    D3D12_RESOURCE_STATE_COMMON,
                    nullptr,
                    IID_PPV_ARGS(&vidMemBuffer)));

    ok = ok && SUCCEEDED(d3d12->getDevice()->CreateCommittedResource(
                    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                    D3D12_HEAP_FLAG_NONE,
                    &CD3DX12_RESOURCE_DESC::Buffer(totalSize),
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&uploadHeapBuffer)));

    ok = ok && SUCCEEDED(d3d12->getDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
    ok = ok && SUCCEEDED(d3d12->getDevice()->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE	, nullptr, IID_PPV_ARGS(&commandList)));

    if(ok)
    {
        vidMemBuffer->SetName(L"StaticBuffer_VidMem");
        uploadHeapBuffer->SetName(L"StaticBuffer_UploadHeap");

        D3D12_RANGE readRange = { 0, 0 };
        uploadHeapBuffer->Map(0, &readRange, reinterpret_cast<void**>(&data));
        offset = 0;
        vidInit = 0;
    }

    return ok;
}

bool ModuleStaticBuffer::allocVertexBuffer(UINT numVertices, UINT strideInBytes, const void* initData, D3D12_VERTEX_BUFFER_VIEW& view)
{
    if(allocBuffer(numVertices * strideInBytes, initData, view.BufferLocation, view.SizeInBytes))
    {
        view.StrideInBytes = strideInBytes;
        return true;
    }

    return false;
}

bool ModuleStaticBuffer::allocIndexBuffer(UINT numIndices, UINT strideInBytes, const void* initData, D3D12_INDEX_BUFFER_VIEW& view)
{
    if(allocBuffer(numIndices * strideInBytes, initData, view.BufferLocation, view.SizeInBytes))
    {
        const DXGI_FORMAT formats[3] = {DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R32_UINT};

        view.Format = formats[strideInBytes >> 1];

        return true;
    }

    return false;

}

bool ModuleStaticBuffer::allocConstantBuffer(UINT size, const void* initData, D3D12_CONSTANT_BUFFER_VIEW_DESC& viewDesc)
{
    return allocBuffer(size, initData, viewDesc.BufferLocation, viewDesc.SizeInBytes);
}

bool ModuleStaticBuffer::allocBuffer(UINT size, const void *initData, D3D12_GPU_VIRTUAL_ADDRESS& bufferLocation, UINT& outSize)
{
    size = alignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    if (offset + size > totalSize)
        return false;

    if (initData)
    {
        memcpy(data + offset, initData, size);
    }

    bufferLocation = vidMemBuffer->GetGPUVirtualAddress() + offset;
    outSize = size;
    offset += size;

    return true;
}

void ModuleStaticBuffer::uploadData()
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12CommandQueue* queue = d3d12->getDrawCommandQueue();

    commandList->Reset(commandAllocator.Get(), nullptr);
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vidMemBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
    commandList->CopyBufferRegion(vidMemBuffer.Get(), vidInit, uploadHeapBuffer.Get(), vidInit, offset - vidInit);
    commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vidMemBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER));
    commandList->Close();

    ID3D12CommandList *commandLists[] = {commandList.Get()};
    queue->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);

    vidInit = offset;
}

void ModuleStaticBuffer::reset()
{
    offset = 0;
    vidInit = 0;
}