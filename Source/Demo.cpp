#include "Globals.h"

#include "Demo.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleRingBuffer.h"
#include "ModuleCamera.h"

#include "DebugDrawPass.h"
#include "ImGuiPass.h"
#include "RenderMeshPass.h"

#include "Scene.h"

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
        renderMeshesPass = std::make_unique<RenderMeshPass>();

        ok = ok && renderMeshesPass->init();
    }

    _ASSERT_EXPR(ok, "Error creating Demo");

    return ok;
}

bool Demo::cleanUp() 
{
    imguiPass.reset();
    debugDrawPass.reset();
    scene.reset();

    return true;
}

void Demo::update() 
{
    // TODO: update animations
    scene->updateWorldTransforms();
}

void Demo::preRender()
{
    imguiPass->startFrame();

    debugDrawCommands();
    imGuiDrawCommands();
}

void Demo::debugDrawCommands()
{
    if (showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    if (showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 2.0f);
}

void Demo::imGuiDrawCommands()
{

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

void Demo::renderDebugDraw(ID3D12GraphicsCommandList *commandList)
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ModuleCamera* camera = app->getCamera();

    unsigned width = d3d12->getWindowWidth();
    unsigned height = d3d12->getWindowHeight();

    debugDrawPass->record(commandList, width, height, camera->getView(), ModuleCamera::getPerspectiveProj(float(width) / float(height)));
}

void Demo::renderImGui(ID3D12GraphicsCommandList *commandList)
{
    imguiPass->record(commandList);
}

void Demo::renderMeshes(ID3D12GraphicsCommandList *commandList)
{
    ModuleRingBuffer* ringBuffer = app->getRingBuffer();

    PerFrame perFrameData = {};
    perFrameData.numDirectionalLights = 0;
    perFrameData.numPointLights = 0;
    perFrameData.numSpotLights = 0;
    perFrameData.numRoughnessLevels = 0;
    perFrameData.cameraPosition = app->getCamera()->getPosition();

    scene->getRenderList(renderList);

    renderMeshesPass->render(renderList, ringBuffer->allocBuffer(&perFrameData), app->getCamera()->getViewProjection(), scene->getSkybox()->getTextureTableDesc(), commandList);
}

void Demo::render()
{
    ModuleD3D12* d3d12 = app->getD3D12();

    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();
    commandList->Reset(d3d12->getCommandAllocator(), nullptr);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    setRenderTarget(commandList);
    renderDebugDraw(commandList);
    renderImGui(commandList);
    renderMeshes(commandList);

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

    bool ok = scene->load("Assets/Models/busterDrone/busterDrone.gltf", "Assets/Models/busterDrone");
    ok = ok && scene->loadSkyboxHDR("Assets/Textures/footprint_court.hdr");

    _ASSERT_EXPR(ok, "Error loading scene");

    return scene != nullptr;
}