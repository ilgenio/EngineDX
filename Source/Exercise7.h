#pragma once

#include "DebugDrawPass.h"
#include "ImGuiPass.h"
#include "Module.h"
#include <imgui.h>
#include "ImGuizmo.h"
#include "BasicMaterial.h"

class Model;

class Exercise7 : public Module
{
    struct PerInstance
    {
        Matrix modelMat;
        Matrix normalMat;

        PBRPhongMaterialData material;
    };

    struct PerFrame
    {
        Vector3 L = Vector3::UnitX;
        float pad0;
        Vector3 Lc = Vector3::One;
        float pad1;
        Vector3 Ac = Vector3::Zero;
        float pad2;
        Vector3 viewPos = Vector3::Zero;
        float pad3;
    };

    struct Light
    {
        Vector3 L = Vector3::One*(-0.5f);
        Vector3 Lc = Vector3::One;
        Vector3 Ac = Vector3::One*(0.1f);
    };

    Light                               light;
    ComPtr<ID3D12RootSignature>         rootSignature;
    ComPtr<ID3D12PipelineState>         pso;
    std::unique_ptr<DebugDrawPass>      debugDrawPass;
    std::unique_ptr<ImGuiPass>          imguiPass;
    bool                                showAxis = false;
    bool                                showGrid = true;
    bool                                showGuizmo = false;
    ImGuizmo::OPERATION                 moOperation = ImGuizmo::TRANSLATE;
    std::unique_ptr<Model>              model;
    ComPtr<ID3D12Resource>              renderTexture;
    ComPtr<ID3D12Resource>              renderDS;


    UINT                                srvTarget = 0;
    UINT                                rtvTarget = 0;
    UINT                                dsvTarget = 0;
    ImVec2                              canvasSize;
    ImVec2                              previousSize;
    ImVec2                              canvasPos;

public:
    Exercise7();
    ~Exercise7();

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void preRender() override;
    virtual void render() override;

private:


    void imGuiCommands();
    bool createRootSignature();
    bool createPSO();
    bool loadModel();
    void renderToTexture(ID3D12GraphicsCommandList* commandList);
    void resizeRenderTexture();
};
