#include "Globals.h"

#include "ModuleDSDescriptors.h"

#include "Application.h"
#include "ModuleD3D12.h"

ModuleDSDescriptors::ModuleDSDescriptors()
{
}

ModuleDSDescriptors::~ModuleDSDescriptors()
{
    handles.forceReleaseDeferred();

    _ASSERT_EXPR((handles.getSize() - handles.getFreeCount()) == 0, "ModuleDSDescriptors has leaks!!!");

}

bool ModuleDSDescriptors::init()
{
    ModuleD3D12* d3d12    = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = MAX_NUM_DEPTHS;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.Type =  D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));

    cpuStart = heap->GetCPUDescriptorHandleForHeapStart();

    return true;
}


UINT ModuleDSDescriptors::create(ID3D12Resource *resource)
{
    UINT handle = handles.allocHandle();

    _ASSERTE(handles.validHandle(handle));

    app->getD3D12()->getDevice()->CreateDepthStencilView(resource, nullptr, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, handles.indexFromHandle(handle), descriptorSize));

    return handle;
}

void ModuleDSDescriptors::release(UINT handle)
{
    if(handle != 0) 
    {
        handles.freeHandle(handle);
    }
}

void ModuleDSDescriptors::deferRelease(UINT handle)
{
    if (handle != 0)
    {
        handles.deferRelease(handle, app->getD3D12()->getCurrentFrame());
    }
}

void ModuleDSDescriptors::preRender()
{
    handles.releaseDeferred(app->getD3D12()->getLastCompletedFrame());
}