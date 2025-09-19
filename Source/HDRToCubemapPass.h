#pragma once

class CubemapMesh;

class HDRToCubemapPass
{

    ComPtr<ID3D12CommandAllocator>    commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12RootSignature>       rootSignature;
    ComPtr<ID3D12RootSignature>       mipsRS;
    ComPtr<ID3D12PipelineState>       pso;
    ComPtr<ID3D12PipelineState>       mipsPSO;
    std::unique_ptr<CubemapMesh>      cubemapMesh;

public:
    HDRToCubemapPass();
    ~HDRToCubemapPass();

    ComPtr<ID3D12Resource> generate(D3D12_GPU_DESCRIPTOR_HANDLE hdrSRV, DXGI_FORMAT format, size_t size);

private:
    bool createRootSignatures();
    bool createPSOs();
};