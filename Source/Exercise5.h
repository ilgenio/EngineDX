#pragma once

#include "DebugDrawPass.h"
#include "ImGuiPass.h"
#include "Module.h"

class Model;

class Exercise5 : public Module
{
    ComPtr<ID3D12RootSignature>     rootSignature;
    ComPtr<ID3D12PipelineState>     pso;
    std::unique_ptr<DebugDrawPass>  debugDrawPass;
    std::unique_ptr<ImGuiPass>      imguiPass;
    bool                            showAxis = true;
    bool                            showGrid = true;
    std::unique_ptr<Model>          model;

public:
    Exercise5();
    ~Exercise5();

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void preRender() override;
    virtual void render() override;

private:

    bool createRootSignature();
    bool createPSO();
    bool createUploadFence();
};
