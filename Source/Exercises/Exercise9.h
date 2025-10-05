#pragma once

#include "Module.h"
#include "ImGuiPass.h"
#include "ShaderTableDesc.h"

class DebugDrawPass;
class SkyboxRenderPass;
class RenderTexture;

// Exercise9 demonstrates rendering a skybox and debug geometry in a DirectX 12 application.
// It manages the setup and rendering of a cubemap-based skybox, debug drawing, and an ImGui-based UI viewer.
// Use this module as a template for advanced scene rendering and UI composition in your graphics pipeline.
class Exercise9 : public Module
{
    ComPtr<ID3D12RootSignature>         rootSignature;
    ComPtr<ID3D12PipelineState>         pso;

    ComPtr<ID3D12Resource>              cubemap;
    std::unique_ptr<DebugDrawPass>      debugDrawPass;
    std::unique_ptr<SkyboxRenderPass>   skyboxRenderPass;

    bool                                showAxis        = true;
    bool                                showGrid        = true;
    ShaderTableDesc                     tableDesc;

public:

    Exercise9();
    ~Exercise9();

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void preRender() override;
    virtual void render() override;

private:

};
