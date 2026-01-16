#pragma once

#include "Module.h"
#include "ShaderTableDesc.h"

class ImGuiPass;

class StartMenu : public Module
{
    unsigned selectedDemo = 0;
public:
    StartMenu();
    ~StartMenu();

    virtual void preRender() override;  

};
