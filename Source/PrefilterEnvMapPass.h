#pragma once

 class CubemapMesh;

class PrefilterEnvMapPass
{
    struct PrefilterConstants
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

    bool init();
    ComPtr<ID3D12Resource> generate(UINT cubeMapDesc, size_t size, UINT mipLevels);

private:

    bool createRootSignature();
    bool createPSO();
};