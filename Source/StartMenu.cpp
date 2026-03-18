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
    ModuleD3D12* d3d12 = app->getD3D12();
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
    
    debugDesc = app->getShaderDescriptors()->allocTable();
    imguiPass = std::make_unique<ImGuiPass>(d3d12->getDevice(), d3d12->getHWnd(), debugDesc.getCPUHandle(1), debugDesc.getGPUHandle(1));
    
    return true;
}

void StartMenu::preRender()
{
    imguiPass->startFrame();

    ImGui::SetNextWindowSize(ImVec2(750, 750), ImGuiCond_FirstUseEver);
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
        app->setDemo(selectedDemo);
    }

    ImGui::End();
}

void StartMenu::render()
{
    ModuleD3D12* d3d12 = app->getD3D12();

    // gets command list and assigns descritpor heaps
    ID3D12GraphicsCommandList* commandList = d3d12->beginFrameRender();

    // Set backbuffer render target and transition to RT
    d3d12->setBackBufferRenderTarget(Vector4(0.3f, 0.3f, 0.3f, 1.0f));

    // ImGui rendering
    imguiPass->record(commandList);

    // Transition to Present, command list Close + queue 
    d3d12->endFrameRender();
}
