#pragma once

#include "Globals.h"

#include <array>
#include <vector>

class Module;
class ModuleRender;
class ModuleD3D12;
class ModuleCamera;
class ModuleRender;

class Application
{
public:

	Application(int argc, wchar_t** argv, void* hWnd);
	~Application();

	bool         init();
	UpdateStatus update();
	bool         cleanUp();

    ModuleD3D12*  getD3D12() { return d3d12; }
    ModuleCamera* getCamera() { return camera;  }
    ModuleRender* getRender() { return render;  }

    //float             getFPS() const { return 1000.0f * float(MAX_FPS_TICKS) / tickSum; }
    //float             getAvgElapsedMs() const { return tickSum / float(MAX_FPS_TICKS); }
    //uint32_t          getElapsedMilis() const { return elapsedMilis; }

private:
    enum { MAX_FPS_TICKS = 30 };
    typedef std::array<uint32_t, MAX_FPS_TICKS> TickList;

    std::vector<Module*> modules;

    ModuleD3D12* d3d12 = nullptr;
    ModuleCamera* camera = nullptr;
    ModuleRender* render = nullptr;

    uint32_t        lastMilis = 0;
    TickList        tickList;
    uint32_t        tickIndex;
    uint32_t        tickSum = 0;
    uint32_t        elapsedMilis = 0;
};

extern Application* app;