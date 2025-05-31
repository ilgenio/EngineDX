#include "Globals.h"

#include "ModuleRenderTargets.h"

#include "Application.h"
#include "ModuleD3D12.h"

ModuleRenderTargets::ModuleRenderTargets()
{
}

ModuleRenderTargets::~ModuleRenderTargets()
{
}

bool ModuleRenderTargets::init()
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    count          = 256;
    current        = 0;
    descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = count;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.Type =  D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));

    cpuStart = heap->GetCPUDescriptorHandleForHeapStart();
    gpuStart = heap->GetGPUDescriptorHandleForHeapStart();

    return true;
}

bool ModuleRenderTargets::cleanUp()
{
    return true;
}

UINT ModuleRenderTargets::createRTV(ID3D12Resource *resource)
{
    _ASSERTE(current < count);

    if(current < count)
    {
        UINT index = current++;
        app->getD3D12()->getDevice()->CreateRenderTargetView(resource, nullptr, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, index, descriptorSize));

        return index;
    }

    return current;
}

