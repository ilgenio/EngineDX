#include "Globals.h"

#include "Demo.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleShaderDescriptors.h"

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

void Demo::preRender() 
{

}

void Demo::render() 
{

}

bool Demo::loadScene()
{
    scene = std::make_unique<Scene>();
    scene->load("Assets/Models/busterDrone/busterDrone.gltf", "Assets/Models/busterDrone");

    return scene != nullptr;
}