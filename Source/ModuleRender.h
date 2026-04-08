#pragma once

#include "Module.h"
#include "ShaderTableDesc.h"

#include<memory>
#include<vector>

class DebugDrawPass;
class ImGuiPass;
class RenderMeshPass;
class GBufferExportPass;
class DeferredPass;
class RenderTexture;
class SkinningPass;
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
        UINT pad; // Padding to ensure 16-byte alignment
        Matrix proj;
        Matrix invView;
    };

    // Passes
    std::unique_ptr<DebugDrawPass>      debugDrawPass;
    std::unique_ptr<ImGuiPass>          imguiPass;
    std::unique_ptr<RenderMeshPass>     renderMeshPass;
    std::unique_ptr<GBufferExportPass>  gbufferPass;
    std::unique_ptr<DeferredPass>       deferredPass;
    std::unique_ptr<SkinningPass>       skinningPass;

    // Render Data
    std::vector<RenderMesh>             renderList;
    D3D12_GPU_VIRTUAL_ADDRESS           perFrameAddress = {};
    D3D12_GPU_VIRTUAL_ADDRESS           skinningAddress = {};
    D3D12_GPU_VIRTUAL_ADDRESS           lightsAddress[3] = {};
    std::unique_ptr<RenderTexture> renderTexture;

    bool showAxis = false;
    bool showGrid = true;
    bool showSceneDebug = false;
    bool showQuadTree = false;
    bool trackFrustum = false;
    bool showGuizmo = false;

    ImVec2 canvasSize;
    ImVec2 canvasPos;
    Vector4 frustumPlanes[6];
    ShaderTableDesc debugDesc;

    BoundingFrustum trackedFrustum;
    UINT quadTreeLevel = 0;

    typedef std::function<void(ID3D12GraphicsCommandList* commandList, const Matrix& view, const Matrix& proj)> Callback;

    std::vector<Callback> renderCallbacks;

public:

    typedef Callback RenderCallback;

    ModuleRender();
    ~ModuleRender();

    void serialize(Json& obj) const;
    void deserialize(const Json& obj);
    
    virtual bool init() override;
    virtual bool cleanUp() override;

    virtual void preRender() override;
    virtual void render() override;

    void addRenderCallback(const RenderCallback& callback) { renderCallbacks.push_back(callback); }
    void clearRenderCallbacks() { renderCallbacks.clear(); }

private:

    void renderToTexture(ID3D12GraphicsCommandList* commandList, const Matrix& view, const Matrix& proj);
    void renderMeshesForward(ID3D12GraphicsCommandList* commandList, const Matrix& view, const Matrix& proj);
    void renderDeferred(ID3D12GraphicsCommandList* commandList);

    void updatePerFrameBuffer(const Matrix& view, const Matrix& projection, const Matrix& invView);
    void updateSkinning(ID3D12GraphicsCommandList* commandList);

    void updateLightsList(ID3D12GraphicsCommandList* commandList);

    void debugDrawCommands();
    void imGuiDrawCommands();
};