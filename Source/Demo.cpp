#include "Globals.h"

#include "Demo.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleSamplers.h"
#include "ModuleRingBuffer.h"
#include "ModuleCamera.h"

#include "DebugDrawPass.h"
#include "ImGuiPass.h"
#include "RenderMeshPass.h"
#include "SkyboxRenderPass.h"

#include "Scene.h"
#include "Model.h"
#include "Skybox.h"
#include "AnimationClip.h"
#include "RenderTexture.h"

Demo::Demo()
{

}

Demo::~Demo()
{

}

bool Demo::init() 
{
    const bool useMSAA = true;

    bool ok = loadScene(useMSAA);
    if (ok)
    {
        ModuleD3D12* d3d12 = app->getD3D12();

        debugDesc = app->getShaderDescriptors()->allocTable();

        debugDrawPass = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getDrawCommandQueue(), useMSAA, debugDesc.getCPUHandle(0), debugDesc.getGPUHandle(0));
        imguiPass = std::make_unique<ImGuiPass>(d3d12->getDevice(), d3d12->getHWnd(), debugDesc.getCPUHandle(1), debugDesc.getGPUHandle(1));
        renderTexture = std::make_unique<RenderTexture>("Exercise12", DXGI_FORMAT_R8G8B8A8_UNORM, Vector4(0.188f, 0.208f, 0.259f, 1.0f), DXGI_FORMAT_D32_FLOAT, 1.0f, useMSAA, useMSAA);
        renderMeshPass = std::make_unique<RenderMeshPass>();

        ok = ok && renderMeshPass->init(useMSAA);
    }

    if (ok)
    {
        ModuleCamera* camera = app->getCamera();

        /*
        camera->setPolar(XMConvertToRadians(-117.0f));
        camera->setAzimuthal(XMConvertToRadians(-1.22f));
        camera->setPanning(Vector3(-24.0f, 2.95f, -6.95f)); */

        camera->setPolar(XMConvertToRadians(1.30f));
        camera->setAzimuthal(XMConvertToRadians(-11.61));
        camera->setPanning(Vector3(0.0f, 1.24f, 4.65f));
    }

    _ASSERT_EXPR(ok, "Error creating Demo");

    return ok;
}

bool Demo::cleanUp() 
{
    imguiPass.reset();
    debugDrawPass.reset();

    skybox.reset();
    model.reset();
    scene.reset();

    return true;
}

void Demo::update() 
{
    // TODO: update animations
    scene->updateAnimations(float(app->getElapsedMilis()) * 0.001f);
    scene->updateWorldTransforms();
}

void Demo::preRender()
{
    imguiPass->startFrame();

    ImGuiID dockspace_id = ImGui::GetID("MyDockNodeId");
    ImGui::DockSpaceOverViewport(dockspace_id);

    static bool init = true;
    if (init)
    {
        init = false;
        ImVec2 mainSize = ImGui::GetMainViewport()->Size;

        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_CentralNode);
        ImGui::DockBuilderSetNodeSize(dockspace_id, mainSize);

        ImGuiID dock_id_left = 0, dock_id_right = 0;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.75f, &dock_id_left, &dock_id_right);
        ImGui::DockBuilderDockWindow("Demo Viewer Options", dock_id_right);
        ImGui::DockBuilderDockWindow("Scene", dock_id_left);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    imGuiDrawCommands();    
    debugDrawCommands();

    if (canvasSize.x > 0.0f && canvasSize.y > 0.0f)
        renderTexture->resize(unsigned(canvasSize.x), unsigned(canvasSize.y));
}

void Demo::debugDrawCommands()
{
    if (showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    if (showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 2.0f);

    char lTmp[1024];
    sprintf_s(lTmp, 1023, "FPS: [%d]. Avg. elapsed (Ms): [%g] ", uint32_t(app->getFPS()), app->getAvgElapsedMs());
    dd::screenText(lTmp, ddConvert(Vector3(10.0f, 10.0f, 0.0f)), dd::colors::White, 0.6f);

}

void Demo::imGuiDrawCommands()
{
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();

    ImGui::Begin("Demo Viewer Options");
    ImGui::Separator();
    ImGui::Text("FPS: [%d]. Avg. elapsed (Ms): [%g] ", uint32_t(app->getFPS()), app->getAvgElapsedMs());
    ImGui::Separator();
    ImGui::Checkbox("Show grid", &showGrid);
    ImGui::Checkbox("Show axis", &showAxis);

    ImGui::Separator();
    ModuleCamera* camera = app->getCamera();
    ImGui::Text("Camera pos: [%.2f, %.2f, %.2f], Camera spherical angles: [%.2f, %.2f]", camera->getPos().x, camera->getPos().y, camera->getPos().z,
        XMConvertToDegrees(camera->getPolar()), XMConvertToDegrees(camera->getAzimuthal()));

    ImGui::Separator();

    ImGui::End();

    bool viewerFocused = false;
    ImGui::Begin("Scene");
    const char* frameName = "Scene Frame";
    ImGuiID id(10);

    ImVec2 max = ImGui::GetWindowContentRegionMax();
    ImVec2 min = ImGui::GetWindowContentRegionMin();
    canvasPos = min;
    canvasSize = ImVec2(max.x - min.x, max.y - min.y);
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    ImGui::BeginChildFrame(id, canvasSize, ImGuiWindowFlags_NoScrollbar);
    viewerFocused = ImGui::IsWindowFocused();

    if (renderTexture->isValid())
    {
        ImGui::Image((ImTextureID)renderTexture->getSrvHandle().ptr, canvasSize);
    }

    ImGui::EndChildFrame();
    ImGui::End();

    app->getCamera()->setEnable(viewerFocused);
}

void Demo::renderMeshes(ID3D12GraphicsCommandList *commandList, const Matrix& view, const Matrix& projection)
{
    ModuleRingBuffer* ringBuffer = app->getRingBuffer();

    PerFrame perFrameData = {};
    perFrameData.numDirectionalLights = 0;
    perFrameData.numPointLights = 0;
    perFrameData.numSpotLights = 0;
    perFrameData.numRoughnessLevels = skybox->getNumIBLMipLevels()  ;
    perFrameData.cameraPosition = app->getCamera()->getPos();

    scene->getRenderList(renderList);

    renderMeshPass->render(commandList, renderList, ringBuffer->allocBuffer(&perFrameData), skybox->getIBLTable(), view*projection);
}

void Demo::renderToTexture(ID3D12GraphicsCommandList* commandList)
{
    ModuleCamera* camera = app->getCamera();

    float aspect = float(renderTexture->getWidth()) / float(renderTexture->getHeight());
    const Matrix& view = camera->getView();
    Matrix proj = ModuleCamera::getPerspectiveProj(aspect);

    BEGIN_EVENT(commandList, "Demo Render Scene to Texture");

    renderTexture->beginRender(commandList);

    skybox->render(commandList, aspect);

    renderMeshes(commandList, view, proj);

    debugDrawPass->record(commandList, renderTexture->getWidth(), renderTexture->getHeight(), view, proj);

    renderTexture->endRender(commandList);

    END_EVENT(commandList);
}

void Demo::render()
{
    ModuleD3D12* d3d12 = app->getD3D12();

    ID3D12GraphicsCommandList* commandList = d3d12->beginFrameRender();

    if (renderTexture->isValid() && canvasSize.x > 0.0f && canvasSize.y > 0.0f)
    {
        renderToTexture(commandList);
    }

    d3d12->setBackBufferRenderTarget(); 

    imguiPass->record(commandList);

    d3d12->endFrameRender();
}

bool Demo::loadScene(bool useMSAA)
{
    scene = std::make_unique<Scene>();


    model.reset(scene->loadModel("Assets/Models/busterDrone/busterDrone.gltf", "Assets/Models/busterDrone"));

    bool ok = model.get();

    if (ok)
    {
        animation = std::make_shared<AnimationClip>();
        animation->load("Assets/Models/busterDrone/busterDrone.gltf", 0);

        model->PlayAnim(animation);
    }
    
    
    //model.reset(scene->loadModel("Assets/Models/BistroExterior/BistroExterior.gltf", "Assets/Models/BistroExterior/"));    
    //model.reset(scene->loadModel("Assets/Models/CompareAmbientOcclusion/CompareAmbientOcclusion.gltf", "Assets/Models/CompareAmbientOcclusion/"));

    skybox = std::make_unique<Skybox>();
    
    //ok = ok && skybox->init("Assets/Textures/qwantani_moon_noon_puresky_4k.hdr", useMSAA);
    ok = ok && skybox->init("Assets/Textures/san_giuseppe_bridge_4k.hdr", useMSAA);

    _ASSERT_EXPR(ok, L"Error loading scene");

    return scene != nullptr;
}
