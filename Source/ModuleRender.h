#pragma once
#include "Module.h"
#include "ShaderTableDesc.h"

class DebugDrawPass;
class ImGuiPass;

class ModuleRender : public Module
{
public:
	ModuleRender();
	~ModuleRender();

	bool init() override;
    void preRender() override;
    void render() override;
	bool cleanUp() override;


private:

    void updateImGui();

private:

    std::unique_ptr<DebugDrawPass> debugDrawPass;
    ShaderTableDesc debugTableDesc;

    std::unique_ptr<ImGuiPass> imguiPass;

	// commands
    ComPtr<ID3D12GraphicsCommandList>   commandList;

    bool                                showGrid = true;
    bool                                showAxis = true;
};

