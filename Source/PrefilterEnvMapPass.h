#pragma once

 class CubemapMesh;

// PrefilterEnvMapPass encapsulates the process of generating a prefiltered environment map from a cubemap texture in a DirectX 12 application.
// It manages command list setup, root signature, pipeline state, and mesh resources required for environment map prefiltering.
// Use this class to create mipmapped specular reflection maps for physically based rendering (PBR) workflows.
class PrefilterEnvMapPass
{
    struct Constants
    {
        float   roughness;
        UINT    samples;
        INT     cubeMapSize;
        INT     lodBias;
    };

    ComPtr<ID3D12CommandAllocator>      commandAllocator;
    ComPtr<ID3D12GraphicsCommandList>   commandList;
    ComPtr<ID3D12RootSignature>         rootSignature;
    ComPtr<ID3D12PipelineState>         pso;
    std::unique_ptr<CubemapMesh>        cubemapMesh;

public:
    PrefilterEnvMapPass();
    ~PrefilterEnvMapPass();

    ComPtr<ID3D12Resource> generate(D3D12_GPU_DESCRIPTOR_HANDLE cubemapSRV, size_t cubeSize, size_t prefilterSize, UINT mipLevels);

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