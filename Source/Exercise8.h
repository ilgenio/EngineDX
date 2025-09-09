#pragma once

#include "DebugDrawPass.h"
#include "ImGuiPass.h"
#include "Module.h"
#include <imgui.h>
#include "ImGuizmo.h"
#include "BasicMaterial.h"

class Model;
class RenderTexture;

class Exercise8 : public Module
{
    struct Ambient
    {
        Vector3 Lc;
    };

    struct Directional
    {
        Vector3 Ld;
        float   intenisty;
        Vector3 Lc;
    };

    struct Point
    {
        Vector3 Lp;
        float   sqRadius;
        Vector3 Lc;
        float   intensity;
    };

    struct Spot
    {
        Vector3 Ld;
        float  sqRadius;
        Vector3 Lp;
        float  inner;
        Vector3 Lc;
        float  outer;
        float  intensity;
    };

    struct PerInstance
    {
        Matrix modelMat;
        Matrix normalMat;

        PBRPhongMaterialData material;
    };

    struct PerFrame
    {
        Ambient ambient;       // Ambient Colour

        uint32_t numDirLights;
        uint32_t numPointLights;
        uint32_t numSpotLights;
        uint32_t pad[2];

        Vector3 viewPos;
    };

    struct Light
    {
        Vector3 L = Vector3::One*(-0.5f);
        Vector3 Lc = Vector3::One;
        Vector3 Ac = Vector3::One*(0.1f);
    };

    enum ELightType { LIGHT_DIRECTIONAL, LIGHT_POINT, LIGHT_SPOT };

    Ambient                             ambient;
    Directional                         dirLight;
    Point                               pointLight;
    Spot                                spotLight;
    ELightType                          lightType = LIGHT_DIRECTIONAL;
    bool                                ddLight = false;
    float                               ddDistance = 2.0f;
    float                               ddSize = 1.0f;

    ComPtr<ID3D12RootSignature>         rootSignature;
    ComPtr<ID3D12PipelineState>         pso;
    std::unique_ptr<DebugDrawPass>      debugDrawPass;
    std::unique_ptr<ImGuiPass>          imguiPass;
    bool                                showAxis = false;
    bool                                showGrid = true;
    bool                                showGuizmo = false;
    ImGuizmo::OPERATION                 moOperation = ImGuizmo::TRANSLATE;
    std::unique_ptr<Model>              model;


    std::unique_ptr<RenderTexture>      renderTexture;
    ImVec2                              canvasSize;
    ImVec2                              canvasPos;

public:
    Exercise8();
    ~Exercise8();

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void preRender() override;
    virtual void render() override;

private:
    void imGuiCommands();
    void imGuiDirection(Vector3& dir);
    void imGuiDirectional(Directional& dirLight);
    void imGuiPoint(Point& pointLight);
    void imGuiSpot(Spot& spotLight);

    void ddDirectional(Directional& dirLight, float distance, float size);
    void ddPoint(Point& pointLight);
    void ddSpot(Spot& spotLight);

    bool createRootSignature();
    bool createPSO();
    bool loadModel();
    void renderToTexture(ID3D12GraphicsCommandList* commandList);
};
