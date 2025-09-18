#include "Globals.h"

#include "ModuleShaderDescriptors.h"

#include "Application.h"
#include "ModuleD3D12.h"

#include "SingleDescriptors.h"
#include "TableDescriptors.h"

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
    heapDesc.NumDescriptors = SingleDescriptors::NUM_DESCRIPTORS + TableDescriptors::NUM_DESCRIPTORS * TableDescriptors::DESCRIPTORS_PER_TABLE;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));

    heap->SetName(L"Module Descriptors Heap");

    UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    singleDescriptors = std::make_unique<SingleDescriptors>(heap, descriptorSize);
    tableDescriptors  = std::make_unique<TableDescriptors>(heap, SingleDescriptors::NUM_DESCRIPTORS, descriptorSize);

    return true;
}

void ModuleShaderDescriptors::preRender()
{
    singleDescriptors->collectGarbage();
    tableDescriptors->collectGarbage();
}

