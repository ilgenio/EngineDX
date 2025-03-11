#include "Globals.h"
#include "ModuleRender.h"

#include "ModuleCamera.h"

#include "DebugDrawPass.h"
#include "ImGuiPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleDescriptors.h"

#include <imgui.h>

ModuleRender::ModuleRender()
{
}

// Destructor
ModuleRender::~ModuleRender()
{
}

bool ModuleRender::init()
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();
    ModuleDescriptors* descriptors = app->getDescriptors();

    debugFontDebugDraw = descriptors->allocateDescriptor();
    debugDrawPass = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getDrawCommandQueue(), descriptors->getCPUHanlde(debugFontDebugDraw), descriptors->getGPUHanlde(debugFontDebugDraw));

    bool ok = SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d3d12->getCommandAllocator(), nullptr, IID_PPV_ARGS(&commandList)));
    ok = ok && SUCCEEDED(commandList->Close());

    debugFontImGUI = descriptors->allocateDescriptor();
    imguiPass     = std::make_unique<ImGuiPass>(d3d12->getDevice(), d3d12->getHWnd());

    return ok;
}

void ModuleRender::preRender()
{
    imguiPass->startFrame();
}

// Called every draw update
void ModuleRender::render()
{
    updateImGui();

    ModuleD3D12* d3d12 = app->getD3D12();
    ModuleCamera* camera = app->getCamera();

    unsigned width = d3d12->getWindowWidth();
    unsigned height = d3d12->getWindowHeight();

    ID3D12CommandAllocator* commandAllocator = d3d12->getCommandAllocator();

    commandList->Reset(commandAllocator, nullptr);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    D3D12_VIEWPORT viewport = { 0, 0, 0.0f, 1.0f, float(width), float(height) };
    D3D12_RECT scissor = { 0, 0, LONG(width), LONG(height)};

    float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = d3d12->getRenderTargetDescriptor();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = d3d12->getDepthStencilDescriptor();

    commandList->OMSetRenderTargets(1, &rtv, false, &dsv);
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    if(showGrid) dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    if(showAxis) dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 1.0f);

    char lTmp[1024];
    sprintf_s(lTmp, 1023, "FPS: [%d]. Avg. elapsed (Ms): [%g] ", uint32_t(app->getFPS()), app->getAvgElapsedMs());
    dd::screenText(lTmp, ddConvert(Vector3(10.0f, 10.0f, 0.0f)), dd::colors::White, 0.6f);

    debugDrawPass->record(commandList.Get(), width, height, camera->getView(), camera->getProj());

    imguiPass->record(commandList.Get());

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier);

    if (SUCCEEDED(commandList->Close()))
    {
        ID3D12CommandList* commandLists[] = { commandList.Get() };
        d3d12->getDrawCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);
    }
}

void ModuleRender::updateImGui()
{
    ImGui::Begin("DebugDraw");
    ImGui::Checkbox("Show Grid", &showGrid);
    ImGui::Checkbox("Show Axis", &showAxis);
    //ImGui::Checkbox("Disable Iridescene", &disableIridescene);
    //ImGui::Combo("Debug Render",  (int*)&debugRender, "None\0Iridiscene Fresnel\0");
    ImGui::End();

    //App->getLevel()->updateImGui();
}

// Called before quitting
bool ModuleRender::cleanUp()
{    
    debugDrawPass.reset();
    imguiPass.reset();

    return true;
}
