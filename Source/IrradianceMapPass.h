#pragma once

class CubemapMesh;

class IrradianceMapPass
{
    ComPtr<ID3D12CommandAllocator>    commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12RootSignature>       rootSignature;
    ComPtr<ID3D12PipelineState>       pso;
    std::unique_ptr<CubemapMesh>      cubemapMesh;

public:
    IrradianceMapPass();
    ~IrradianceMapPass();

    bool init();
    ComPtr<ID3D12Resource> generate(UINT cubeMapDesc, size_t size);

private:
    bool createRootSignature();
    bool createPSO();
};