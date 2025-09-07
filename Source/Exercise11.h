#pragma once

#include "Module.h"
#include <memory>

class CubemapMesh;
class DebugDrawPass;
class ImGuiPass;
class IrradianceMapPass;

class Exercise11 : public Module
{
    ComPtr<ID3D12RootSignature>         rootSignature;
    ComPtr<ID3D12PipelineState>         pso;

    std::unique_ptr<CubemapMesh>        cubemapMesh;
    ComPtr<ID3D12Resource>              cubemap;
    std::unique_ptr<DebugDrawPass>      debugDrawPass;
    std::unique_ptr<ImGuiPass>          imguiPass;
    std::unique_ptr<IrradianceMapPass>  irradianceMapPass;
    ComPtr<ID3D12Resource>              irradianceMap;
    bool showAxis = true;
    bool                                showGrid = true;


    UINT cubemapDesc = 0;
    UINT irradianceMapDesc = 0;
    UINT rtvTarget = 0;

public:

    Exercise11();
    ~Exercise11();

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void render() override;

private:

    bool createRootSignature();
    bool createPSO();
};
