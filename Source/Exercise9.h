#pragma once

#include "Module.h"
#include "ImGuiPass.h"

class CubemapMesh;
class DebugDrawPass;
class RenderTexture;

class Exercise9 : public Module
{
    ComPtr<ID3D12RootSignature>     rootSignature;
    ComPtr<ID3D12PipelineState>     pso;

    std::unique_ptr<CubemapMesh>    cubemapMesh;
    ComPtr<ID3D12Resource>          cubemap;
    std::unique_ptr<DebugDrawPass>  debugDrawPass;
    std::unique_ptr<ImGuiPass>      imguiPass;

    bool showAxis = true;
    bool showGrid = true;
    UINT cubemapDesc = 0;

    std::unique_ptr<RenderTexture>  renderTexture;
    ImVec2 canvasSize;
    ImVec2 canvasPos;

public:

    Exercise9();
    ~Exercise9();

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void preRender() override;
    virtual void render() override;

private:

    void imGuiCommands();

    bool createRootSignature();
    bool createPSO();
    void renderToTexture(ID3D12GraphicsCommandList* commandList);
    void resizeRenderTexture();
};
