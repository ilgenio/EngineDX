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
    AccelerationStructureBuffers createBottomLevelAS(ID3D12Device5* pDevice, ID3D12GraphicsCommandList4* pCmdList, ID3D12Resource* pVB);
    
};