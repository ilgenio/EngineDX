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

void ModuleRender::serialize(Json& obj) const
{
    Json::object renderObj;

    renderObj["showAxis"] = showAxis;
    renderObj["showGrid"] = showGrid;
    renderObj["showSkeleton"] = showSkeleton;
    renderObj["showQuadTree"] = showQuadTree;
    renderObj["trackFrustum"] = trackFrustum;
    renderObj["showGuizmo"] = showGuizmo;

    obj = renderObj;
}

void ModuleRender::deserialize(const Json& obj)
{
    showAxis = obj["showAxis"].bool_value();
    showGrid = obj["showGrid"].bool_value();
    showSkeleton = obj["showSkeleton"].bool_value();
    showQuadTree = obj["showQuadTree"].bool_value();
    trackFrustum = obj["trackFrustum"].bool_value();
    showGuizmo = obj["showGuizmo"].bool_value();
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

        ImGuiID dock_id_left = 0, dock_id_right = 0;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.75f, &dock_id_left, &dock_id_right);
        ImGui::DockBuilderDockWindow("Demo Viewer Options", dock_id_right);
        ImGui::DockBuilderDockWindow("Scene", dock_id_left);

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

    if (showSkeleton)
    {
        // Draw Model transforms
        ModuleScene* sceneModule = app->getScene();

        auto drawNode = [](const char* name, const Matrix& worldT, const Matrix& parentT, void* userData)
            {
                dd::line(ddConvert(worldT.Translation()), ddConvert(parentT.Translation()), dd::colors::White, 0, false);

                Vector3 scale;
                Quaternion rotation;
                Vector3 translation;

                worldT.Decompose(scale, rotation, translation);

                Matrix world = Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(translation);

                dd::axisTriad(ddConvert(world), 0.01f, 0.1f);
            };

        for (UINT modelIdx : debugDrawModels)
        {
            if (modelIdx < sceneModule->getModelCount())
            {
                std::shared_ptr<const Model> model = sceneModule->getModel(modelIdx);
                model->enumerateNodes(drawNode);
            }
        }
    }
}

void ModuleRender::imGuiDrawCommands()
{
    ImGui::Begin("Demo Viewer Options");
    ImGui::Separator();
    ImGui::Text("FPS: [%d]. Avg. elapsed (Ms): [%g] ", uint32_t(app->getFPS()), app->getAvgElapsedMs());
    ImGui::Separator();
    ImGui::Checkbox("Show grid", &showGrid);
    ImGui::Checkbox("Show axis", &showAxis);
    ImGui::Checkbox("Show skeleton", &showSkeleton);
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

    app->getCamera()->setEnableInput(viewerFocused);
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

void ModuleRender::addDebugDrawModel(UINT index)
{
    debugDrawModels.insert(index);
}

void ModuleRender::removeDebugDrawModel(UINT index)
{
    debugDrawModels.erase(index);
}

