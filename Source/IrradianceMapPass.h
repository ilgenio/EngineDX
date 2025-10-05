#pragma once

class CubemapMesh;

// IrradianceMapPass encapsulates the process of generating an irradiance map from a cubemap texture in a DirectX 12 application.
// It manages command list setup, root signature, pipeline state, and mesh resources required for irradiance map rendering.
// Use this class to create diffuse environment lighting maps for physically based rendering (PBR) workflows.
class IrradianceMapPass
{
    struct Constants
    {
        UINT    samples;
        INT     cubeMapSize;
        float   lodBias;
        UINT    padding; // Padding to ensure 16-byte alignment
    };

    ComPtr<ID3D12CommandAllocator>    commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ComPtr<ID3D12RootSignature>       rootSignature;
    ComPtr<ID3D12PipelineState>       pso;
    std::unique_ptr<CubemapMesh>      cubemapMesh;

public:
    IrradianceMapPass();
    ~IrradianceMapPass();

    ComPtr<ID3D12Resource> generate(D3D12_GPU_DESCRIPTOR_HANDLE cubemapSRV, size_t cubeSize, size_t irradianceSize);

private:
    struct SkyParams
    {
        Matrix vp;
        BOOL   flipX;
        BOOL   flipZ;
        UINT   padding[2];
    };

    bool createRootSignature();
    bool createPSO();
};