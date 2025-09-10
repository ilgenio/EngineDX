#include "Globals.h"

#include "ModuleRTDescriptors.h"

#include "Application.h"
#include "ModuleD3D12.h"

ModuleRTDescriptors::ModuleRTDescriptors()
{
}

ModuleRTDescriptors::~ModuleRTDescriptors()
{
    handles.forceReleaseDeferred();

#ifdef _DEBUG
    size_t allocCount = handles.getSize() - handles.getFreeCount();

    if (allocCount > 0)
    {
        LOG("ModuleRTDescriptors has leaks: %u/%u handles used\n", allocCount, handles.getSize());
    }
#endif
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

UINT ModuleRTDescriptors::create(ID3D12Resource *resource, UINT arraySlice, DXGI_FORMAT format)
{
    UINT handle = handles.allocHandle();

    _ASSERTE(handles.validHandle(handle));

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = format;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
    rtvDesc.Texture2DArray.ArraySize = 1;
    rtvDesc.Texture2DArray.FirstArraySlice = arraySlice;
    rtvDesc.Texture2DArray.MipSlice = 0;
    rtvDesc.Texture2DArray.PlaneSlice = 0;

    app->getD3D12()->getDevice()->CreateRenderTargetView(resource, &rtvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, handles.indexFromHandle(handle), descriptorSize));

    return handle;

}

void ModuleRTDescriptors::release(UINT handle)
{
    if(handle != 0) 
    {
        handles.freeHandle(handle);
    }
}

void ModuleRTDescriptors::deferRelease(UINT handle)
{
    handles.deferRelease(handle, app->getD3D12()->getCurrentFrame());
}

void ModuleRTDescriptors::preRender()
{
    handles.releaseDeferred(app->getD3D12()->getLastCompletedFrame());
}
