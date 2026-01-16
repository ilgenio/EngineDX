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

void StartMenu::preRender()
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

