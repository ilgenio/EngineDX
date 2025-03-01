#pragma once

class DescriptorGroup
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
    uint32_t descCount = 0;
    uint32_t descSize  = 0;

public:

    DescriptorGroup() {}

    void SetHandles(D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu, uint32_t count, uint32_t size);

    D3D12_CPU_DESCRIPTOR_HANDLE getCPU(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE getGPU(uint32_t index) const;

    uint32_t getCount() const { return descCount; }
    uint32_t getSize() const { return descSize; }
};

class StaticDescriptorHeap
{
public:
    void init(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t descriptorCount);

    bool allocate(uint32_t count, DescriptorGroup& descriptor);

    ID3D12DescriptorHeap* getHeap() {return heap.Get();}
    uint32_t getCount() const {return count;}

private:
    uint32_t current = 0;
    uint32_t count = 0;
    uint32_t size = 0;
    bool     shaderVisible = false;

    ComPtr<ID3D12DescriptorHeap> heap;
};

