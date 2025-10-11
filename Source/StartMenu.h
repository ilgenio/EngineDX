#pragma once

#include "Module.h"
#include "ShaderTableDesc.h"

class ImGuiPass;

class StartMenu : public Module
{
    std::unique_ptr<ImGuiPass> imguiPass;
    ShaderTableDesc debugDesc;
    unsigned selectedDemo = 0;
public:
    StartMenu();
    ~StartMenu();

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void preRender() override;  
    virtual void render() override;

private:
    void imGuiDrawCommands();
};
