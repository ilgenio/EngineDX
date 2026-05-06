#pragma once

#include "Module.h"
#include "ShaderTableDesc.h"
#include "ImGuizmo.h"
#include "RenderStructs.h"

#include<memory>
#include<vector>

class DebugDrawPass;
class ImGuiPass;
class RenderMeshPass;
class GBufferExportPass;
class DeferredPass;
class RenderTexture;
class SkinningPass;
class BuildTileLightsPass;
class DecalPass;
class ShadowMapPass;
struct RenderMesh;  
struct RenderData;

class ModuleRender : public Module
{
    // Passes
    std::unique_ptr<DebugDrawPass>       debugDrawPass;
    std::unique_ptr<ImGuiPass>           imguiPass;
    std::unique_ptr<RenderMeshPass>      renderMeshPass;
    std::unique_ptr<GBufferExportPass>   gbufferPass;
    std::unique_ptr<DeferredPass>        deferredPass;
    std::unique_ptr<SkinningPass>        skinningPass;
    std::unique_ptr<BuildTileLightsPass> buildTileLightsPass;
    std::unique_ptr<DecalPass>           decalPass;     
    std::unique_ptr<ShadowMapPass>       shadowMapPass;    

    // Render Data
    std::vector<RenderMesh>             renderList;
    std::vector<RenderMesh>             shadowCasters;
    RenderData                          renderData;
    std::unique_ptr<RenderTexture>      renderTexture;

    bool showGuizmo = false;
    Matrix guizmoTransform = Matrix::Identity;
    ImGuizmo::OPERATION gizmoOperation = ImGuizmo::TRANSLATE;

    ImVec2 canvasSize;
    ImVec2 canvasPos;
    ShaderTableDesc debugDesc;

    typedef std::function<void(ID3D12GraphicsCommandList* commandList, const Matrix& view, const Matrix& proj)> Callback;

    std::vector<Callback> renderCallbacks;

public:

    typedef Callback RenderCallback;

    ModuleRender();
    ~ModuleRender();

    virtual bool init() override;
    virtual bool cleanUp() override;

    virtual void preRender() override;
    virtual void render() override;

    void addRenderCallback(const RenderCallback& callback) { renderCallbacks.push_back(callback); }
    void clearRenderCallbacks() { renderCallbacks.clear(); }

    // Guizmo management stuff 
    void setShowGuizmo(bool show) { showGuizmo = show; }
    void setGuizmoOperation(ImGuizmo::OPERATION operation) { gizmoOperation = operation; }
    ImGuizmo::OPERATION getGuizmoOperation() const { return gizmoOperation; }
    void setGuizmoTransform(const Matrix& transform) { guizmoTransform = transform;  }
    const Matrix& getGuizmoTransform() const { return  guizmoTransform; }

    float getRenderTargetAspect() const;

    Vector4 computeShadowBoundingSphere() const;

private:

    void renderGBuffer(ID3D12GraphicsCommandList* commandList);
    void updatePerFrameData();
    void updateLightsList(ID3D12GraphicsCommandList* commandList);
    void renderToTexture(ID3D12GraphicsCommandList* commandList);
    void executeCommands(ID3D12GraphicsCommandList* commandList);

    void imGuiDrawCommands();
};