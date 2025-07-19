#pragma once

#include "Module.h"
#include <memory>

class CubemapMesh;

class Exercise9 : public Module
{
    ComPtr<ID3D12RootSignature>  rootSignature;
    ComPtr<ID3D12PipelineState>  pso;

    std::unique_ptr<CubemapMesh> cubemapMesh;
    ComPtr<ID3D12Resource>       cubemap;

    UINT cubemapDesc = 0;

public:

    Exercise9();
    ~Exercise9();

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void render() override;

private:


    bool createRootSignature();
    bool createPSO();
};
