#pragma once

#include "Module.h"
#include "BasicMaterial.h"
#include "ShaderTableDesc.h"

class BasicModel;
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
    struct PerFrame
    {
        Vector3 camPos;
        float   roughnessLevels;
    };

    struct PerInstance
    {
        Matrix modelMat;
        Matrix normalMat;

        MetallicRoughnessMaterialData material;
    };

    ComPtr<ID3D12RootSignature>             rootSignature;
    ComPtr<ID3D12PipelineState>             pso;

    std::unique_ptr<DebugDrawPass>          debugDrawPass;
    std::unique_ptr<ImGuiPass>              imguiPass;
    std::unique_ptr<IrradianceMapPass>      irradianceMapPass;
    std::unique_ptr<SkyboxRenderPass>       skyboxRenderPass;

    ComPtr<ID3D12Resource>                  irradianceMap;
    ComPtr<ID3D12Resource>                  skybox;

    std::unique_ptr<BasicModel>             model;

    bool showAxis = true;
    bool showGrid = true;

    enum TexSlots
    {
        TEX_SLOT_IMGUI = 0,
        TEX_SLOT_CUBEMAP = 1,
        TEX_SLOT_IRRADIANCE = 2,
        TEX_SLOT_IBL = TEX_SLOT_IRRADIANCE,
        TEX_SLOT_COUNT = 6
    };

    ShaderTableDesc tableDesc;

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
    void renderToTexture(ID3D12GraphicsCommandList* commandList);

    bool createRootSignature();
    bool createPSO();
    bool loadModel();
};
