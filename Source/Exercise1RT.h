#pragma once 

#include "Module.h"

struct AccelerationStructureBuffers
{
    ComPtr<ID3D12Resource> pScratch;
    ComPtr<ID3D12Resource> pResult;
    ComPtr<ID3D12Resource> pInstanceDesc;    // Used only for top-level AS
};

class Exercise1RT : public Module
{
public:
    virtual bool init() override;
    virtual UpdateStatus update() override;

private:
    AccelerationStructureBuffers createBottomLevelAS(ID3D12GraphicsCommandList4* pCmdList, ID3D12Resource* pVB, DXGI_FORMAT format, size_t stride, size_t vertexCount);
    AccelerationStructureBuffers createTopLevelAS(ComPtr<ID3D12Resource>* pBottomLevelAS, size_t bottomLevelCount, uint64_t& tlasSize);
    ComPtr<ID3D12Resource> createBuffer(uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES &heapProps);
};