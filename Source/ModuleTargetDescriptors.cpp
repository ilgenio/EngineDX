#include "Globals.h"

#include "ModuleTargetDescriptors.h"

#include "Application.h"
#include "ModuleD3D12.h"

ModuleTargetDescriptors::ModuleTargetDescriptors()
{
}

ModuleTargetDescriptors::~ModuleTargetDescriptors()
{
    _ASSERTE(rtHandles.getFreeCount() == rtHandles.getSize());
    _ASSERTE(dsHandles.getFreeCount() == dsHandles.getSize());
}

bool ModuleTargetDescriptors::init()
{
    ModuleD3D12* d3d12    = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    rtDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dsDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = MAX_NUM_TARGETS;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.Type =  D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heapRT));
    cpuStartRT = heapRT->GetCPUDescriptorHandleForHeapStart();

    heapDesc.NumDescriptors = MAX_NUM_DEPTHS;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heapDS));

    cpuStartDS = heapDS->GetCPUDescriptorHandleForHeapStart();


    return true;
}


RenderTargetDesc ModuleTargetDescriptors::createRT(ID3D12Resource *resource)
{
    UINT handle = rtHandles.allocHandle();

    _ASSERTE(rtHandles.validHandle(handle));

    app->getD3D12()->getDevice()->CreateRenderTargetView(resource, nullptr, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStartRT, rtHandles.indexFromHandle(handle), rtDescriptorSize));

    return RenderTargetDesc(handle, &rtRefCounts[indexFromRTHandle(handle)]);
}

RenderTargetDesc ModuleTargetDescriptors::createRT(ID3D12Resource *resource, UINT arraySlice, UINT mipSlice, DXGI_FORMAT format)
{
    UINT handle = rtHandles.allocHandle();

    _ASSERTE(rtHandles.validHandle(handle));

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = format;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
    rtvDesc.Texture2DArray.ArraySize = 1;
    rtvDesc.Texture2DArray.FirstArraySlice = arraySlice;
    rtvDesc.Texture2DArray.MipSlice = mipSlice;
    rtvDesc.Texture2DArray.PlaneSlice = 0;

    app->getD3D12()->getDevice()->CreateRenderTargetView(resource, &rtvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStartRT, rtHandles.indexFromHandle(handle), rtDescriptorSize));

    return RenderTargetDesc(handle, &rtRefCounts[indexFromRTHandle(handle)]);

}

void ModuleTargetDescriptors::releaseRT(UINT handle)
{
    if(handle != 0) 
    {
        rtHandles.freeHandle(handle);
    }
}

DepthStencilDesc ModuleTargetDescriptors::createDS(ID3D12Resource* resource)
{
    UINT handle = dsHandles.allocHandle();

    _ASSERTE(dsHandles.validHandle(handle));

    app->getD3D12()->getDevice()->CreateDepthStencilView(resource, nullptr, CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStartDS, dsHandles.indexFromHandle(handle), dsDescriptorSize));

    return DepthStencilDesc(handle, &dsRefCounts[indexFromDSHandle(handle)]);
}

void ModuleTargetDescriptors::releaseDS(UINT handle)
{
    if (handle != 0)
    {
        dsHandles.freeHandle(handle);
    }
}


