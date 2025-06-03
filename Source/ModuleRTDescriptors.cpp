#include "Globals.h"

#include "ModuleRTDescriptors.h"

#include "Application.h"
#include "ModuleD3D12.h"

ModuleRTDescriptors::ModuleRTDescriptors()
{
}

ModuleRTDescriptors::~ModuleRTDescriptors()
{
}

bool ModuleRTDescriptors::init()
{
    ModuleD3D12* d3d12    = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = MAX_NUM_TARGETS;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.Type =  D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));

    cpuStart = heap->GetCPUDescriptorHandleForHeapStart();

    return true;
}


UINT ModuleRTDescriptors::create(ID3D12Resource *resource)
{
    UINT handle = handles.allocHandle();

    _ASSERTE(handles.validHandle(handle));

    app->getD3D12()->getDevice()->CreateRenderTargetView(resource, nullptr, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, handles.indexFromHandle(handle), descriptorSize));

    return handle;
}

void ModuleRTDescriptors::release(UINT handle)
{
    if(handle != 0) 
    {
        handles.freeHandle(handle);
    }
}


