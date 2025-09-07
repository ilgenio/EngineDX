#pragma once

#include "Module.h"
#include "ImGuiPass.h"
#include <imgui.h>
#include <memory>

class CubemapMesh;
class DebugDrawPass;

class Exercise9 : public Module
{
    ComPtr<ID3D12RootSignature>     rootSignature;
    ComPtr<ID3D12PipelineState>     pso;

    std::unique_ptr<CubemapMesh>    cubemapMesh;
    ComPtr<ID3D12Resource>          cubemap;
    std::unique_ptr<DebugDrawPass>  debugDrawPass;
    std::unique_ptr<ImGuiPass>      imguiPass;
    ComPtr<ID3D12Resource>          renderTexture;
    ComPtr<ID3D12Resource>          renderDS;

    bool showAxis = true;
    bool showGrid = true;
    UINT cubemapDesc = 0;
    UINT srvTarget = 0;
    UINT rtvTarget = 0;
    UINT dsvTarget = 0;

    ImVec2 canvasSize;
    ImVec2 previousSize;
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
