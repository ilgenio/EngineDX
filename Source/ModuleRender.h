#pragma once

#include "Module.h"
#include "ShaderTableDesc.h"

#include<memory>
#include<vector>

class DebugDrawPass;
class ImGuiPass;
class RenderMeshPass;
class RenderTexture;
struct RenderMesh;

class ModuleRender : public Module
{
    struct PerFrame
    {
        UINT numDirectionalLights = 0;
        UINT numPointLights = 0;
        UINT numSpotLights = 0;
        UINT numRoughnessLevels = 0;
        Vector3 cameraPosition;
    };

    std::unique_ptr<DebugDrawPass>    debugDrawPass;
    std::unique_ptr<ImGuiPass>        imguiPass;
    std::unique_ptr<RenderMeshPass>   renderMeshPass;

    std::vector<RenderMesh>           renderList;

    bool showAxis = false;
    bool showGrid = false;
    bool showQuadTree = false;
    bool trackFrustum = true;
    bool showGuizmo = false;

    ImVec2 canvasSize;
    ImVec2 canvasPos;
    Vector4 frustumPlanes[6];
    std::unique_ptr<RenderTexture> renderTexture;
    ShaderTableDesc debugDesc;

    BoundingFrustum trackedFrustum;
    UINT quadTreeLevel = 0;

public:
    ModuleRender();
    ~ModuleRender();

    virtual bool init() override;
    virtual bool cleanUp() override;

    virtual void preRender() override;
    virtual void render() override;

private:
    void renderToTexture(ID3D12GraphicsCommandList* commandList);
    void renderMeshes(ID3D12GraphicsCommandList* commandList, const Matrix& view, const Matrix& projection);

    void debugDrawCommands();
    void imGuiDrawCommands();
};