#pragma once

#include "Module.h"

#include "ShaderTableDesc.h"

class Scene;
class DebugDrawPass;
class ImGuiPass;
class RenderTexture;

class Demo : public Module
{
    std::unique_ptr<DebugDrawPass>  debugDrawPass;
    std::unique_ptr<ImGuiPass>      imguiPass;

    std::unique_ptr<Scene>          scene;

    bool showAxis = true;
    bool showGrid = true;

    ShaderTableDesc debugDesc;
public:

    Demo();
    ~Demo();

    virtual bool init() override;
    virtual bool cleanUp() override;
    virtual void preRender() override;
    virtual void render() override;

private:
    bool loadScene();
};
