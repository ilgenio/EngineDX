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

    RenderTargetDesc createRTV(ID3D12Resource* resource);
    RenderTargetDesc createRTV(ID3D12Resource* resource, UINT arraySlice, UINT mipSlice, DXGI_FORMAT format);
    DepthStencilDesc createDSV(ID3D12Resource* resource);

private:

    void releaseRTV(UINT handle);
    D3D12_CPU_DESCRIPTOR_HANDLE getRTVCPUHandle(UINT handle) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStartRT, rtHandles.indexFromHandle(handle), rtDescriptorSize); }

    void releaseDSV(UINT handle);
    D3D12_CPU_DESCRIPTOR_HANDLE getDSVCPUHandle(UINT handle) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStartDS, dsHandles.indexFromHandle(handle), dsDescriptorSize); }


    bool isValidRTV(UINT handle) const { return rtHandles.validHandle(handle); }
    UINT indexFromRTVHandle(UINT handle) const { return rtHandles.indexFromHandle(handle); }

    bool isValidDSV(UINT handle) const { return dsHandles.validHandle(handle); }
    UINT indexFromDSVHandle(UINT handle) const { return dsHandles.indexFromHandle(handle); }

private:
    enum { MAX_NUM_TARGETS = 256, MAX_NUM_DEPTHS = 128 };

    typedef HandleManager<MAX_NUM_TARGETS> RTVHandles;
    typedef HandleManager<MAX_NUM_DEPTHS> DSVHandles;
   
    ComPtr<ID3D12DescriptorHeap> heapRT;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStartRT = {0};
    UINT rtDescriptorSize = 0;

    ComPtr<ID3D12DescriptorHeap> heapDS;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuStartDS = { 0 };
    UINT dsDescriptorSize = 0;

    RTVHandles rtHandles;
    DSVHandles dsHandles;

    std::array<UINT, MAX_NUM_TARGETS> rtRefCounts = { 0 };
    std::array<UINT, MAX_NUM_DEPTHS> dsRefCounts = { 0 };
};

