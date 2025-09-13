#pragma once

class CubemapMesh;

// IrradianceMapPass encapsulates the process of generating an irradiance map from a cubemap texture in a DirectX 12 application.
// It manages command list setup, root signature, pipeline state, and mesh resources required for irradiance map rendering.
// Use this class to create diffuse environment lighting maps for physically based rendering (PBR) workflows.
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