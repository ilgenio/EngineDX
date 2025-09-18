#pragma once

#include "Module.h"
#include "BasicMaterial.h"

class SphereMesh;
class DebugDrawPass;
class ImGuiPass;
class IrradianceMapPass;
class PrefilterEnvMapPass;
class EnvironmentBRDFPass;
class SkyboxRenderPass;
class HDRToCubemapPass;
class RenderTexture;

class Exercise11 : public Module
{
    struct PerInstance
    {
        Matrix modelMat;
        Matrix normalMat;

        MetallicRoughnessMaterialData material;
    };

    ComPtr<ID3D12RootSignature>         sphereRS;
    ComPtr<ID3D12PipelineState>         spherePSO;

    std::unique_ptr<SphereMesh>             sphereMesh;
    std::unique_ptr<DebugDrawPass>          debugDrawPass;
    std::unique_ptr<ImGuiPass>              imguiPass;
    std::unique_ptr<IrradianceMapPass>      irradianceMapPass;
    std::unique_ptr<PrefilterEnvMapPass>    prefilterEnvMapPass;
    std::unique_ptr<EnvironmentBRDFPass>    environmentBRDFPass ;
    std::unique_ptr<SkyboxRenderPass>       skyboxRenderPass;
    std::unique_ptr<HDRToCubemapPass>       hdrToCubemapPass;

    ComPtr<ID3D12Resource>                  hdrSky;
    ComPtr<ID3D12Resource>                  irradianceMap;
    ComPtr<ID3D12Resource>                  prefilteredEnvMap;
    ComPtr<ID3D12Resource>                  environmentBRDF;
    ComPtr<ID3D12Resource>                  skybox;

    bool showAxis = true;
    bool showGrid = true;

    // SRV descriptors
    UINT imguiTextDesc = 0;
    UINT hdrSkyDesc = 0;
    UINT iblTableDesc = 0;
    UINT prefilteredEnvMapDesc = 0;
    UINT environmentBRDFDesc = 0;

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

    bool createSphereRS();
    bool createSpherePSO();
};
