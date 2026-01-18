#include "Globals.h"

#include "ModuleRender.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleScene.h"
#include "ModuleCamera.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleRingBuffer.h"

#include "Scene.h"
#include "Skybox.h"

#include "DebugDrawPass.h"
#include "ImGuiPass.h"
#include "RenderMeshPass.h"
#include "RenderTexture.h"
#include "SkinningPass.h"


ModuleRender::ModuleRender()
{

}

ModuleRender::~ModuleRender()
{

}

bool ModuleRender::init()
{
    ModuleD3D12* d3d12 = app->getD3D12();

    debugDesc       = app->getShaderDescriptors()->allocTable();

    debugDrawPass   = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getDrawCommandQueue(), false, debugDesc.getCPUHandle(0), debugDesc.getGPUHandle(0));
    imguiPass       = std::make_unique<ImGuiPass>(d3d12->getDevice(), d3d12->getHWnd(), debugDesc.getCPUHandle(1), debugDesc.getGPUHandle(1));
    renderTexture   = std::make_unique<RenderTexture>("ModuleRender", DXGI_FORMAT_R8G8B8A8_UNORM, Vector4(0.188f, 0.208f, 0.259f, 1.0f), DXGI_FORMAT_D32_FLOAT, 1.0f, false, false);
    renderMeshPass  = std::make_unique<RenderMeshPass>();
    skinningPass    = std::make_unique<SkinningPass>();

    bool ok = renderMeshPass->init(false);

    return ok;

}

bool ModuleRender::cleanUp()
{
    imguiPass.reset();
    debugDrawPass.reset();

    return true;
}

void ModuleRender::preRender()
{
    // Frustum culling
    if (renderTexture->isValid())
    {
        float aspect = float(renderTexture->getWidth()) / float(renderTexture->getHeight());

        Vector4 planes[6];
        app->getCamera()->getFrustumPlanes(planes, aspect, false);

        renderList.clear();
        app->getScene()->getScene()->frustumCulling(planes, renderList);
    }

    // ImGui and DebugDraw commands

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

    if (canvasSize.x > 0.0f && canvasSize.y > 0.0f)
    {
        renderTexture->resize(unsigned(canvasSize.x), unsigned(canvasSize.y));

    }

    ModuleD3D12* d3d12 = app->getD3D12();
    unsigned width = d3d12->getWindowWidth();
    unsigned height = d3d12->getWindowHeight();

    ModuleScene* scene = app->getScene();

    if (scene && scene->getModelCount() > 0)
    {
        imGuiDrawCommands();
        debugDrawCommands();
    }
}

void ModuleRender::debugDrawCommands()
{
    if (showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    if (showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 2.0f);

    char lTmp[1024];
    sprintf_s(lTmp, 1023, "FPS: [%d]. Avg. elapsed (Ms): [%g] ", uint32_t(app->getFPS()), app->getAvgElapsedMs());
    dd::screenText(lTmp, ddConvert(Vector3(10.0f, 10.0f, 0.0f)), dd::colors::White, 0.6f);

    if (trackFrustum && renderTexture->isValid())
    {
        float aspect = float(renderTexture->getWidth()) / float(renderTexture->getHeight());
        app->getCamera()->getFrustumPlanes(frustumPlanes, aspect, true);
        trackedFrustum = app->getCamera()->getFrustum(aspect);
    }

    if (showQuadTree)
    {
        app->getScene()->getScene()->debugDrawQuadTree(frustumPlanes, quadTreeLevel);
        
        Vector3 points[8];
        trackedFrustum.GetCorners(points);
        dd::box(ddConvert(points), dd::colors::White);
    }
}

void ModuleRender::imGuiDrawCommands()
{
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();

    ImGui::Begin("Demo Viewer Options");
    ImGui::Separator();
    ImGui::Text("FPS: [%d]. Avg. elapsed (Ms): [%g] ", uint32_t(app->getFPS()), app->getAvgElapsedMs());
    ImGui::Separator();
    ImGui::Checkbox("Show grid", &showGrid);
    ImGui::Checkbox("Show axis", &showAxis);
    ImGui::Checkbox("Show quadtree", &showQuadTree);
    if (showQuadTree)
    {
        ImGui::SliderInt("QuadTree level", (int*)&quadTreeLevel, 0, 10);
    }
    ImGui::Checkbox("Track frustum", &trackFrustum);

    ImGui::Separator();
    ModuleCamera* camera = app->getCamera();
    ImGui::Text("Camera pos: [%.2f, %.2f, %.2f], Camera spherical angles: [%.2f, %.2f]", camera->getPos().x, camera->getPos().y, camera->getPos().z,
        XMConvertToDegrees(camera->getPolar()), XMConvertToDegrees(camera->getAzimuthal()));

    ImGui::Separator();

    ImGui::Text("Renedering %d meshes", renderList.size());

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
}

void ModuleRender::renderMeshes(ID3D12GraphicsCommandList *commandList, const Matrix& view, const Matrix& projection)
{
    ModuleRingBuffer* ringBuffer = app->getRingBuffer();
    Skybox* skybox = app->getScene()->getSkybox();

    if (!renderList.empty() && skybox->isValid())
    {
        PerFrame perFrameData = {};
        perFrameData.numDirectionalLights = 0;
        perFrameData.numPointLights = 0;
        perFrameData.numSpotLights = 0;
        perFrameData.numRoughnessLevels = skybox->getNumIBLMipLevels();
        perFrameData.cameraPosition = app->getCamera()->getPos();

        renderMeshPass->render(commandList, renderList, ringBuffer->alloc(&perFrameData), skybox->getIBLTable(), view * projection);
    }
}

void ModuleRender::renderToTexture(ID3D12GraphicsCommandList* commandList)
{
    ModuleCamera* camera = app->getCamera();

    float aspect = float(renderTexture->getWidth()) / float(renderTexture->getHeight());
    const Matrix& view = camera->getView();
    Matrix proj = ModuleCamera::getPerspectiveProj(aspect);

    BEGIN_EVENT(commandList, "Render Scene to Texture");

    renderTexture->beginRender(commandList);

    app->getScene()->getSkybox()->render(commandList, aspect);

    renderMeshes(commandList, view, proj);

    debugDrawPass->record(commandList, renderTexture->getWidth(), renderTexture->getHeight(), view, proj);

    renderTexture->endRender(commandList);

    END_EVENT(commandList);
}

void ModuleRender::render()
{
    ModuleD3D12* d3d12 = app->getD3D12();

    ID3D12GraphicsCommandList* commandList = d3d12->beginFrameRender();

    skinningPass->record(commandList, std::span<RenderMesh>(renderList.data(), renderList.size()));

    if (renderTexture->isValid() && canvasSize.x > 0.0f && canvasSize.y > 0.0f)
    {
        renderToTexture(commandList);
    }

    d3d12->setBackBufferRenderTarget(); 

    imguiPass->record(commandList);

    d3d12->endFrameRender();
}
