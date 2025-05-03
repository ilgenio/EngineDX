#pragma once

#include "DebugDrawPass.h"
#include "ImGuiPass.h"
#include "Module.h"
#include <imgui.h>
#include "ImGuizmo.h"
#include "BasicMaterial.h"

class Model;

class Exercise6 : public Module
{
    struct PerInstance
    {
        Matrix modelMat;
        Matrix normalMat;

        PhongMaterialData material;
    };

    struct PerFrame
    {
        Vector3 L = Vector3::UnitX;
        Vector3 Lc = Vector3::One;
        Vector3 Ac = Vector3::Zero;

    } perFrame;

    ComPtr<ID3D12RootSignature>     rootSignature;
    ComPtr<ID3D12PipelineState>     pso;
    std::unique_ptr<DebugDrawPass>  debugDrawPass;
    std::unique_ptr<ImGuiPass>      imguiPass;
    bool                            showAxis = false;
    bool                            showGrid = true;
    bool                            showGuizmo = true;
    ImGuizmo::OPERATION             gizmoOperation = ImGuizmo::TRANSLATE;
    std::unique_ptr<Model>          model;

public:
    Exercise6();
    ~Exercise6();

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void preRender() override;
    virtual void render() override;

private:


    void imGuiCommands();
    bool createRootSignature();
    bool createPSO();
    bool loadModel();
};
