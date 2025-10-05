#include "Globals.h"

#include "StartMenu.h"
#include "ImGuiPass.h"  

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleSamplers.h"

#include "DemoDescriptors.h"


StartMenu::StartMenu()
{

}

StartMenu::~StartMenu()
{

}

bool StartMenu::init()
{
    ModuleD3D12 *d3d12 = app->getD3D12();

    debugDesc = app->getShaderDescriptors()->allocTable();

    imguiPass = std::make_unique<ImGuiPass>(d3d12->getDevice(), d3d12->getHWnd(), debugDesc.getCPUHandle(0), debugDesc.getGPUHandle(0));
    return imguiPass != nullptr;
}

bool StartMenu::cleanUp()
{
    imguiPass.reset();
    return true;
}

void StartMenu::preRender()
{
    imguiPass->startFrame();

    imGuiDrawCommands();
}

void StartMenu::setRenderTarget(ID3D12GraphicsCommandList *commandList)
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

void StartMenu::render()
{
    ModuleD3D12* d3d12 = app->getD3D12();

    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();
    commandList->Reset(d3d12->getCommandAllocator(), nullptr);

    ID3D12DescriptorHeap* descriptorHeaps[] = { app->getShaderDescriptors()->getHeap(), app->getSamplers()->getHeap() };
    commandList->SetDescriptorHeaps(2, descriptorHeaps);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    setRenderTarget(commandList);
    imguiPass->record(commandList);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier);

    if (SUCCEEDED(commandList->Close()))
    {
        ID3D12CommandList* commandLists[] = { commandList };
        d3d12->getDrawCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);
    }

}

void StartMenu::imGuiDrawCommands()
{
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Start Menu");

    std::span<DemoDescriptors> demos = getDemoDescriptors();

    ImGui::ListBox("Select Demo", (int*)&selectedDemo, [](void* data, int idx) -> const char*
        {
            static char tmp[512];

            std::span<DemoDescriptors>* demos = (std::span<DemoDescriptors>*)data;

            sprintf_s(&tmp[0], 511, "%s - %s", (*demos)[idx].name, (*demos)[idx].description);
            return tmp;            
        }, (void*)&demos, int(demos.size()), int(demos.size()));

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Description:");
    ImGui::TextWrapped("%s", demos[selectedDemo].description);
    ImGui::Separator();
    if (ImGui::Button("Start Demo"))
    {
        app->swapModule(this, demos[selectedDemo].createFunc());
    }

    ImGui::End();
}