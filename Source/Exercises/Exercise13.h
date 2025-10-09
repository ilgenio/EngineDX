#pragma once

#include "Module.h"
#include "BasicMaterial.h"
#include "ShaderTableDesc.h"

class BasicModel;
class DebugDrawPass;
class ImGuiPass;
class RenderTexture;
class Skybox;

class Exercise13 : public Module
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
    std::unique_ptr<Skybox>                 skybox;


    std::unique_ptr<BasicModel>             model;

    bool showAxis = false;
    bool showGrid = false;

    enum TexSlots
    {
        TEX_SLOT_IMGUI = 0,
        TEX_SLOT_DEBUGDRAW,
    };

    enum RootParams
    {
        ROOTPARAM_MVP = 0,
        ROOTPARAM_PERFRAME,
        ROOTPARAM_PERINSTANCE,
        ROOTPARAM_IBL_TABLE,
        ROOTPARAM_MATERIAL_TABLE,
        ROOTPARAM_SAMPLERS,
        ROOTPARAM_COUNT
    };

    ShaderTableDesc debugDesc;

    std::unique_ptr<RenderTexture> renderTexture;
    ImVec2 canvasSize;

public:

    Exercise13();
    ~Exercise13();

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void preRender() override;
    virtual void render() override;

private:
    void configureDockspace();
    void imGuiCommands();
    void debugCommands();
    void renderToTexture(ID3D12GraphicsCommandList* commandList);
    void renderModel(ID3D12GraphicsCommandList *commandList);

    bool createRootSignature();
    bool createPSO();
    bool loadModel();
};
