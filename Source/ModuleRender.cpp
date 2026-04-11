#include "Globals.h"

#include "ModuleRender.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleScene.h"
#include "ModuleCamera.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleRingBuffer.h"
#include "ModuleDynamicBuffer.h"

#include "Scene.h"
#include "Model.h"
#include "Skybox.h"
#include "Light.h"

#include "DebugDrawPass.h"
#include "ImGuiPass.h"
#include "RenderMeshPass.h"
#include "GBufferExportPass.h"
#include "DeferredPass.h"
#include "RenderTexture.h"
#include "SkinningPass.h"

#include "json_utils.h"


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
    renderTexture   = std::make_unique<RenderTexture>("ModuleRender", DXGI_FORMAT_R8G8B8A8_UNORM, Vector4(0.188f, 0.208f, 0.259f, 1.0f), DXGI_FORMAT_UNKNOWN, 1.0f, false, false);
    renderMeshPass  = std::make_unique<RenderMeshPass>();
    gbufferPass     = std::make_unique<GBufferExportPass>();
    deferredPass    = std::make_unique<DeferredPass>();
    skinningPass    = std::make_unique<SkinningPass>();

    bool ok = renderMeshPass->init(false);
    ok = ok && gbufferPass->init();
    ok = ok && deferredPass->init();

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

        ImGuiID dock_id_left = 0, dock_id_center = 0, dock_id_right = 0;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, &dock_id_left, &dockspace_id);
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, &dock_id_right, &dock_id_center);
        ImGui::DockBuilderDockWindow("Viewer Properties", dock_id_right);
        ImGui::DockBuilderDockWindow("Scene", dock_id_center);
        ImGui::DockBuilderDockWindow("Objects", dock_id_left);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    if (canvasSize.x > 0.0f && canvasSize.y > 0.0f)
    {
        UINT sizeX = UINT(canvasSize.x);
        UINT sizeY = UINT(canvasSize.y);
        renderTexture->resize(sizeX, sizeY);
        gbufferPass->resize(sizeX, sizeY);
    }

    ModuleD3D12* d3d12 = app->getD3D12();
    unsigned width = d3d12->getWindowWidth();
    unsigned height = d3d12->getWindowHeight();

    ModuleScene* scene = app->getScene();

    if (scene && scene->getModelCount() > 0)
    {
        imGuiDrawCommands();
    }
}

void ModuleRender::imGuiDrawCommands()
{
    ImGui::Begin("Viewer Properties");
    ImGui::Separator();
    ImGui::Text("FPS: [%d]. Avg. elapsed (Ms): [%g] ", uint32_t(app->getFPS()), app->getAvgElapsedMs());
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

    if (showGuizmo)
    {
        ImGuizmo::BeginFrame();

        if (ImGui::IsKeyPressed(ImGuiKey_T)) gizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) gizmoOperation = ImGuizmo::ROTATE;

        const Matrix& viewMatrix = camera->getView();
        Matrix projMatrix = ModuleCamera::getPerspectiveProj(float(canvasSize.x) / float(canvasSize.y));

        ImGuizmo::SetRect(cursorPos.x, cursorPos.y, canvasSize.x, canvasSize.y);
        ImGuizmo::SetDrawlist();
        ImGuizmo::Manipulate((const float*)&viewMatrix, (const float*)&projMatrix, gizmoOperation, ImGuizmo::LOCAL, (float*)&guizmoTransform);
    }


    ImGui::EndChildFrame();
    ImGui::End();

    camera->setEnableInput(viewerFocused && !ImGuizmo::IsUsing());
}

void ModuleRender::updatePerFrameBuffer(const Matrix& view, const Matrix& projection, const Matrix& invView)
{
    Matrix viewProj = view * projection;

    Scene* scene = app->getScene()->getScene();

    PerFrame perFrameData = {};
    perFrameData.numDirectionalLights = UINT(scene->getDirectionalLights().size());
    perFrameData.numPointLights = UINT(scene->getPointLights().size());
    perFrameData.numSpotLights = UINT(scene->getSpotLights().size());
    perFrameData.numRoughnessLevels = app->getScene()->getSkybox()->getNumIBLMipLevels();
    perFrameData.cameraPosition = app->getCamera()->getPos();
    perFrameData.proj = projection.Transpose();
    perFrameData.invView = invView.Transpose();

    perFrameAddress = app->getRingBuffer()->alloc(&perFrameData);
}

void ModuleRender::updateSkinning(ID3D12GraphicsCommandList* commandList)
{
    // Do skinnging
    skinningPass->record(commandList, std::span<RenderMesh>(renderList.data(), renderList.size()));

    // Update current skinning Buffer
    skinningAddress = skinningPass->getOutputAddress(app->getD3D12()->getCurrentBackBufferIdx());
}

void ModuleRender::updateLightsList(ID3D12GraphicsCommandList* commandList)
{
    ModuleDynamicBuffer* dynamicBuffer = app->getDynamicBuffer();

    auto buildData = [=]<typename T>(std::span<T*> list) -> D3D12_GPU_VIRTUAL_ADDRESS
    {
        std::vector<T> data;
        data.reserve(list.size());

        for (T* light : list)
        {
            data.push_back(*light);
        }

        return dynamicBuffer->alloc(data.data(), data.size());
    };

    Scene* scene = app->getScene()->getScene();

    lightsAddress[0] = buildData(scene->getDirectionalLights());
    lightsAddress[1] = buildData(scene->getPointLights());
    lightsAddress[2] = buildData(scene->getSpotLights());

    dynamicBuffer->submitCopy(commandList);
}

void ModuleRender::renderMeshesForward(ID3D12GraphicsCommandList *commandList, const Matrix& view, const Matrix& projection)
{
    Skybox* skybox = app->getScene()->getSkybox();

    if (!renderList.empty() && skybox->isValid())
    {
        renderMeshPass->render(commandList, renderList, skinningAddress, perFrameAddress, skybox->getIBLTable(), view * projection);
    }
}

void ModuleRender::renderDeferred(ID3D12GraphicsCommandList* commandList)
{
    Skybox* skybox = app->getScene()->getSkybox();

    if (skybox->isValid())
    {
        deferredPass->render(commandList, perFrameAddress, gbufferPass->getGBuffer().getSrvTableDesc().getGPUHandle(), lightsAddress, skybox->getIBLTable());
    }
}

void ModuleRender::renderToTexture(ID3D12GraphicsCommandList* commandList, const Matrix& view, const Matrix& proj)
{
    BEGIN_EVENT(commandList, "Render Scene to Texture");

    // Transition to RT + set render target
    D3D12_CPU_DESCRIPTOR_HANDLE sharedDSV = gbufferPass->getGBuffer().getDsvDesc().getCPUHandle();
    renderTexture->beginRender(commandList, &sharedDSV);

    // Deferred pass
    renderDeferred(commandList);

    // Render meshes TODO: In future will be the alpha blend pass
    //renderMeshesForward(commandList, view, proj);

    // Render the skybox
    app->getScene()->getSkybox()->render(commandList, proj);

    // Custom render scene functions (e.g. for demos)
    for (const auto& callback : renderCallbacks)
    {
        callback(commandList, view, proj);
    }

    // Debug Draw
    debugDrawPass->record(commandList, renderTexture->getWidth(), renderTexture->getHeight(), view, proj);

    // Transition to SRV
    renderTexture->endRender(commandList);

    END_EVENT(commandList);
}

void ModuleRender::render()
{
    ModuleD3D12* d3d12 = app->getD3D12();

    // gets command list and assigns descritpor heaps
    ID3D12GraphicsCommandList* commandList = d3d12->beginFrameRender();

    if (renderTexture->isValid() && canvasSize.x > 0.0f && canvasSize.y > 0.0f)
    {
        // Compute Camera matrices
        ModuleCamera* camera = app->getCamera();
        float aspect = float(renderTexture->getWidth()) / float(renderTexture->getHeight());
        const Matrix& invView = camera->getCamera(); // Note: camera matrix is the inverse of the view matrix
        const Matrix& view = camera->getView();
        Matrix proj = ModuleCamera::getPerspectiveProj(aspect);

        // Update PerFrame cbuffer
        updatePerFrameBuffer(view, proj, invView);

        // Runs skinning
        updateSkinning(commandList);

        // Updates light lists buffers
        updateLightsList(commandList);

        // GBuffer Export
        gbufferPass->render(commandList, renderList, skinningAddress, view * proj);

        // Do forward mesh rendering + deferred pass
        renderToTexture(commandList, view, proj);
    }

    // Set backbuffer render target and transition to RT
    d3d12->setBackBufferRenderTarget(); 

    // ImGui rendering
    imguiPass->record(commandList);

    // Transition to Present, command list Close + queue 
    d3d12->endFrameRender();
}

float ModuleRender::getRenderTargetAspect() const
{
    if (renderTexture->isValid())
    {
        return float(renderTexture->getWidth()) / float(renderTexture->getHeight());
    }

    return 0.0f;
}

