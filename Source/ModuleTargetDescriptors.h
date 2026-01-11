#pragma once

#include "Module.h"
#include "HandleManager.h"
#include "RenderTargetDesc.h"
#include "DepthStencilDesc.h"

class ModuleTargetDescriptors : public Module
{
    friend class RenderTargetDesc;
    friend class DepthStencilDesc;

public:

    ModuleTargetDescriptors();
    ~ModuleTargetDescriptors();

    bool init() override;

    RenderTargetDesc createRT(ID3D12Resource* resource);
    RenderTargetDesc createRT(ID3D12Resource* resource, UINT arraySlice, UINT mipSlice, DXGI_FORMAT format);
    DepthStencilDesc createDS(ID3D12Resource* resource);

private:

    void releaseRT(UINT handle);
    D3D12_CPU_DESCRIPTOR_HANDLE getRTCPUHandle(UINT handle) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStartRT, rtHandles.indexFromHandle(handle), rtDescriptorSize); }

    void releaseDS(UINT handle);
    D3D12_CPU_DESCRIPTOR_HANDLE getDSCPUHandle(UINT handle) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStartDS, dsHandles.indexFromHandle(handle), dsDescriptorSize); }


    bool isValidRT(UINT handle) const { return rtHandles.validHandle(handle); }
    UINT indexFromRTHandle(UINT handle) const { return rtHandles.indexFromHandle(handle); }

    bool isValidDS(UINT handle) const { return dsHandles.validHandle(handle); }
    UINT indexFromDSHandle(UINT handle) const { return dsHandles.indexFromHandle(handle); }

private:
    enum { MAX_NUM_TARGETS = 256, MAX_NUM_DEPTHS = 128 };

    typedef HandleManager<MAX_NUM_TARGETS> RTHandles;
    typedef HandleManager<MAX_NUM_DEPTHS> DSHandles;
   
    ComPtr<ID3D12DescriptorHeap> heapRT;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStartRT = {0};
    UINT rtDescriptorSize = 0;

    ComPtr<ID3D12DescriptorHeap> heapDS;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStartDS = { 0 };
    UINT dsDescriptorSize = 0;

    RTHandles rtHandles;
    DSHandles dsHandles;

    std::array<UINT, MAX_NUM_TARGETS> rtRefCounts = { 0 };
    std::array<UINT, MAX_NUM_DEPTHS> dsRefCounts = { 0 };
};

