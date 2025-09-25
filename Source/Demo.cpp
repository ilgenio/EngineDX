#include "Globals.h"

#include "Demo.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleCamera.h"

#include "DebugDrawPass.h"
#include "ImGuiPass.h"

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
    }

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

    debugDraw();
    imGuiDraw();
}

void Demo::debugDraw()
{
    if (showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    if (showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 2.0f);
}

void Demo::imGuiDraw()
{

}

void Demo::render()
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ModuleCamera* camera = app->getCamera();

    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();
    commandList->Reset(d3d12->getCommandAllocator(), nullptr);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

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

    debugDrawPass->record(commandList, width, height, camera->getView(), ModuleCamera::getPerspectiveProj(float(width) / float(height)));

    imguiPass->record(commandList);

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
    scene->load("Assets/Models/busterDrone/busterDrone.gltf", "Assets/Models/busterDrone");

    return scene != nullptr;
}