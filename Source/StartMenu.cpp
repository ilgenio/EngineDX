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
    app->getD3D12()->flush();
    imguiPass.reset();
    return true;
}

void StartMenu::preRender()
{
    imguiPass->startFrame();

    imGuiDrawCommands();
}

void StartMenu::render()
{
    ModuleD3D12* d3d12 = app->getD3D12();

    ID3D12GraphicsCommandList* commandList = d3d12->beginFrameRender();

    d3d12->setBackBufferRenderTarget(Vector4(0.188f, 0.208f, 0.259f, 1.0f));

    imguiPass->record(commandList);

    d3d12->endFrameRender();
}

void StartMenu::imGuiDrawCommands()
{
    ImGui::SetNextWindowSize(ImVec2(750, 500), ImGuiCond_FirstUseEver);
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