#include "Globals.h"

#include "ModuleShaderDescriptors.h"

#include "Application.h"
#include "ModuleD3D12.h"

#include "ShaderTableDesc.h"

ModuleShaderDescriptors::ModuleShaderDescriptors()
{
}

ModuleShaderDescriptors::~ModuleShaderDescriptors()
{
}

bool ModuleShaderDescriptors::init()
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = NUM_DESCRIPTORS * DESCRIPTORS_PER_TABLE;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));

    heap->SetName(L"Shader Descriptors Heap");

    descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    gpuStart = heap->GetGPUDescriptorHandleForHeapStart();
    cpuStart = heap->GetCPUDescriptorHandleForHeapStart();

    return true;
}

void ModuleShaderDescriptors::preRender()
{
    collectGarbage();
}

ShaderTableDesc ModuleShaderDescriptors::allocTable()
{
    UINT handle = alloc();

    _ASSERTE(handles.validHandle(handle));

    return ShaderTableDesc(handle, &refCounts[indexFromHandle(handle)]);
}

void ModuleShaderDescriptors::deferRelease(UINT handle)
{
    if (handle)
    {
        handles.deferRelease(handle, app->getD3D12()->getCurrentFrame());
    }
}

void ModuleShaderDescriptors::collectGarbage()
{
    handles.collectGarbage(app->getD3D12()->getLastCompletedFrame());
}
