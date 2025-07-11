#pragma once

#include "DebugDrawPass.h"
#include "ImGuiPass.h"
#include "Module.h"
#include <imgui.h>
#include "ImGuizmo.h"

class Model;

class Exercise5 : public Module
{
    ComPtr<ID3D12RootSignature>         rootSignature;
    ComPtr<ID3D12PipelineState>         pso;
    std::unique_ptr<DebugDrawPass>      debugDrawPass;
    std::unique_ptr<ImGuiPass>          imguiPass;
    std::vector<ComPtr<ID3D12Resource>> materialBuffers;
    bool                                showAxis = false;
    bool                                showGrid = true;
    bool                                showGuizmo = true;
    ImGuizmo::OPERATION                 gizmoOperation = ImGuizmo::TRANSLATE;
    std::unique_ptr<Model>              model;

public:
    Exercise5();
    ~Exercise5();

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
