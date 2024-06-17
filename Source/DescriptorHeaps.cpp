#include "Globals.h"

#include "DescriptorHeaps.h"

#include "Application.h"
#include "ModuleD3D12.h"

void DescriptorGroup::SetHandles(D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu, uint32_t count, uint32_t size)
{
    cpuHandle = cpu;
    gpuHandle = gpu;
    descCount = count;
    descSize = size;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorGroup::getCPU(uint32_t index) const 
{ 
    assert(index < descCount);

    D3D12_CPU_DESCRIPTOR_HANDLE ret = cpuHandle;
    ret.ptr += index * descSize;

    return ret; 
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorGroup::getGPU(uint32_t index) const 
{
    assert(index < descCount);

    D3D12_GPU_DESCRIPTOR_HANDLE ret = gpuHandle;
    ret.ptr += index * descSize;

    return ret; 
}


void StaticDescriptorHeap::init(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t descriptorCount)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    shaderVisible = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    count = descriptorCount;
    size = device->GetDescriptorHandleIncrementSize(heapType);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = descriptorCount;
    heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.Type = heapType;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));
}

bool StaticDescriptorHeap::allocate(uint32_t numDescriptors, DescriptorGroup& descriptor)
{
    if (current + numDescriptors > count)
    {
        return false;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = heap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += current * size;

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
    if (shaderVisible)
    {
        gpuHandle = heap->GetGPUDescriptorHandleForHeapStart();
        gpuHandle.ptr += current * size;
    }

    current += numDescriptors;

    descriptor.SetHandles(cpuHandle, gpuHandle, numDescriptors, size);

    return true;
}

