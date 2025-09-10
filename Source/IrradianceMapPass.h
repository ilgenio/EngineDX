#pragma once

#include <memory>

class CubemapMesh;

class IrradianceMapPass
{
    ComPtr<ID3D12RootSignature>  rootSignature;
    ComPtr<ID3D12PipelineState>  pso;
    std::unique_ptr<CubemapMesh> cubemapMesh;

public:
    IrradianceMapPass();
    ~IrradianceMapPass();

    ComPtr<ID3D12Resource> record(ID3D12GraphicsCommandList* cmdList, UINT cubeMapDesc, size_t size);

private:
    void createResources(size_t size);
    bool createRootSignature();
    bool createPSO();
};