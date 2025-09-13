#pragma once

#include "Module.h"

class CubemapMesh;
class SphereMesh;
class DebugDrawPass;
class ImGuiPass;
class IrradianceMapPass;
class RenderTexture;

class Exercise11 : public Module
{
    ComPtr<ID3D12RootSignature>         skyRS;
    ComPtr<ID3D12RootSignature>         sphereRS;

    ComPtr<ID3D12PipelineState>         skyPSO;
    ComPtr<ID3D12PipelineState>         spherePSO;

    std::unique_ptr<SphereMesh>         sphereMesh;
    std::unique_ptr<CubemapMesh>        cubemapMesh;
    ComPtr<ID3D12Resource>              cubemap;
    std::unique_ptr<DebugDrawPass>      debugDrawPass;
    std::unique_ptr<ImGuiPass>          imguiPass;
    std::unique_ptr<IrradianceMapPass>  irradianceMapPass;
    ComPtr<ID3D12Resource>              irradianceMap;

    bool showAxis = true;
    bool showGrid = true;

    UINT cubemapDesc = 0;
    UINT imguiTextDesc = 0;
    UINT irradianceMapDesc = 0;

    std::unique_ptr<RenderTexture> renderTexture;
    ImVec2 canvasSize;
    ImVec2 canvasPos;

public:

    Exercise11();
    ~Exercise11();

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void preRender() override;
    virtual void render() override;

private:
    void imGuiCommands();
    void resizeRenderTexture();
    void renderToTexture(ID3D12GraphicsCommandList* commandList);

    bool createSkyRS();
    bool createSkyPSO();

    bool createSphereRS();
    bool createSpherePSO();
};
