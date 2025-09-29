#pragma once

#include "Module.h"

#include "ShaderTableDesc.h"

struct RenderMesh;
class Scene;
class Model;
class Skybox;
class DebugDrawPass;
class ImGuiPass;
class RenderTexture;
class RenderMeshPass;
class SkyboxRenderPass;
class AnimationClip;

class Demo : public Module
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
    std::unique_ptr<SkyboxRenderPass> skyboxPass;

    std::unique_ptr<Scene>            scene;
    std::unique_ptr<Model>            model;
    std::unique_ptr<Skybox>           skybox;

    std::vector<RenderMesh>           renderList;
    std::shared_ptr<AnimationClip>    animation;

    bool showAxis = false;
    bool showGrid = false;

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
    void renderDebugDraw(ID3D12GraphicsCommandList* commandList, UINT width, UINT height, const Matrix& view, const Matrix& projection );
    void renderImGui(ID3D12GraphicsCommandList* commandList);
    void renderMeshes(ID3D12GraphicsCommandList* commandList, const Matrix& view, const Matrix& projection);
    void renderSkybox(ID3D12GraphicsCommandList* commandList, const Quaternion& cameraRot, const Matrix& projection);

    bool loadScene();
    void debugDrawCommands();
    void imGuiDrawCommands();
};
