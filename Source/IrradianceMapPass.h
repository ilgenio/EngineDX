#pragma once

#include <memory>

class CubemapMesh;

class IrradianceMapPass
{
    ComPtr<ID3D12RootSignature>  rootSignature;
    ComPtr<ID3D12PipelineState>  pso;
    std::unique_ptr<CubemapMesh> cubemapMesh;
    ComPtr<ID3D12Resource>       irradianceMap;

public:
    IrradianceMapPass();
    ~IrradianceMapPass();

    void record(ID3D12GraphicsCommandList* cmdList, UINT cubeMapDesc, uint32_t width, uint32_t height);

    ID3D12Resource* getIrradianceMap() const { return irradianceMap.Get(); }

private:

    bool createRootSignature();
    bool createPSO();
};