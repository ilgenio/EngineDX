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
    bool ok = loadScene();
    if (ok)
    {
        ModuleD3D12* d3d12 = app->getD3D12();

        debugDesc = app->getShaderDescriptors()->allocTable();

        debugDrawPass = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getDrawCommandQueue(), debugDesc.getCPUHandle(0), debugDesc.getGPUHandle(0));
        imguiPass = std::make_unique<ImGuiPass>(d3d12->getDevice(), d3d12->getHWnd(), debugDesc.getCPUHandle(1), debugDesc.getGPUHandle(1));
        skyboxPass = std::make_unique<SkyboxRenderPass>();
        renderTexture = std::make_unique<RenderTexture>("Exercise12", DXGI_FORMAT_R8G8B8A8_UNORM, Vector4(0.188f, 0.208f, 0.259f, 1.0f), DXGI_FORMAT_D32_FLOAT, 1.0f);
        renderMeshPass = std::make_unique<RenderMeshPass>();

        ok = ok && renderMeshPass->init();
    }

    if (ok)
    {
        ModuleCamera* camera = app->getCamera();
        camera->setPolar(0.0f);
        camera->setAzimuthal(-0.25f);
        camera->setPanning(Vector3(0.15f, 0.95f, 4.25f));

        /*
        camera->setPolar(-0.85f);
        camera->setAzimuthal(-0.063f);
        camera->setPanning(Vector3(-6.7f, 1.94f, 14.44f));
        */
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

    ImGui::DockSpaceOverViewport();

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

void Demo::setRenderTarget(ID3D12GraphicsCommandList *commandList)
{
    ModuleD3D12* d3d12 = app->getD3D12();

    unsigned width = d3d12->getWindowWidth();
    unsigned height = d3d12->getWindowHeight();

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = d3d12->getRenderTargetDescriptor();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = d3d12->getDepthStencilDescriptor();

    commandList->OMSetRenderTargets(1, &rtv, false, &dsv);

    float clearColor[] = { 0.188f, 0.208f, 0.259f, 1.0f };
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, LONG(width), LONG(height) };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);
}

void Demo::renderDebugDraw(ID3D12GraphicsCommandList *commandList, UINT width, UINT height, const Matrix& view, const Matrix& projection )
{
    debugDrawPass->record(commandList, width, height, view, projection);
}

void Demo::renderImGui(ID3D12GraphicsCommandList *commandList)
{
    imguiPass->record(commandList);
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

void Demo::renderSkybox(ID3D12GraphicsCommandList *commandList, const Quaternion& cameraRot, const Matrix& projection)
{
    ModuleCamera* camera = app->getCamera();

    skyboxPass->record(commandList, skybox->getCubemapSRV(), cameraRot, projection);
}

void Demo::renderToTexture(ID3D12GraphicsCommandList* commandList)
{
    ModuleCamera* camera = app->getCamera();

    const Matrix& view = camera->getView();
    Matrix proj = ModuleCamera::getPerspectiveProj(float(renderTexture->getWidth()) / float(renderTexture->getHeight()));

    BEGIN_EVENT(commandList, "Demo Render Scene to Texture");

    renderTexture->transitionToRTV(commandList);

    renderTexture->setRenderTarget(commandList);

    renderSkybox(commandList, camera->getRot(), proj);
    renderMeshes(commandList, view, proj);
    renderDebugDraw(commandList, renderTexture->getWidth(), renderTexture->getHeight(), view, proj);

    renderTexture->transitionToSRV(commandList);

    END_EVENT(commandList);

}

void Demo::render()
{
    ModuleD3D12* d3d12 = app->getD3D12();

    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();
    commandList->Reset(d3d12->getCommandAllocator(), nullptr);

    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getShaderDescriptors()->getHeap(), app->getSamplers()->getHeap() };
    commandList->SetDescriptorHeaps(2, descriptorHeaps);

    if (renderTexture->isValid() && canvasSize.x > 0.0f && canvasSize.y > 0.0f)
    {
        renderToTexture(commandList);
    }

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    setRenderTarget(commandList);
    renderImGui(commandList);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier);

    if (SUCCEEDED(commandList->Close()))
    {
        ID3D12CommandList* commandLists[] = { commandList };
        d3d12->getDrawCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);
    }
}

bool Demo::loadScene()
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

    skybox = std::make_unique<Skybox>();
    
    ok = ok && skybox->loadHDR("Assets/Textures/footprint_court.hdr");

    _ASSERT_EXPR(ok, L"Error loading scene");

    return scene != nullptr;
}
