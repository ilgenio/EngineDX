#pragma once

#include "Module.h"
#include "ShaderTableDesc.h"

#include <memory>

class ImGuiPass;
class ImGuiPass;


class StartMenu : public Module
{
    unsigned selectedDemo = 0;
    ShaderTableDesc debugDesc;
    std::unique_ptr<ImGuiPass> imguiPass;

public:
    StartMenu();
    ~StartMenu();

    bool init() override;
    void preRender() override;  
    void render() override;
};
