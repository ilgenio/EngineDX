#pragma once

#include "Module.h"

#include "ShaderTableDesc.h"

class Scene;
class DebugDrawPass;
class ImGuiPass;
class RenderTexture;
class RenderMeshPass;

class Demo : public Module
{
    struct PerFrame
    {
        UINT numDirectionalLights = 0;
        UINT numPointLights = 0;
        UINT numSpotLights = 0;
        UINT numRoughnessLevels = 0;
        Vector3 cameraPosition;
        float pad1;
    };

    std::unique_ptr<DebugDrawPass>  debugDrawPass;
    std::unique_ptr<ImGuiPass>      imguiPass;
    std::unique_ptr<RenderMeshPass> renderMeshPass;

    std::unique_ptr<Scene>          scene;
    std::vector<const RenderMesh*>  renderList;

    bool showAxis = true;
    bool showGrid = true;

    ShaderTableDesc debugDesc;
public:

    Demo();
    ~Demo();

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void update() override;
    virtual void preRender() override;
    virtual void render() override;

private:
    void setRenderTarget(ID3D12GraphicsCommandList* commandList);
    void renderDebugDraw(ID3D12GraphicsCommandList* commandList);
    void renderImGui(ID3D12GraphicsCommandList* commandList);
    void renderMeshes(ID3D12GraphicsCommandList* commandList);
    bool loadScene();
    void debugDrawCommands();
    void imGuiDrawCommands();
};
