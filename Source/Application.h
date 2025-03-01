#pragma once

#include "Globals.h"

#include <array>
#include <vector>
#include <chrono>

class Module;
class ModuleRender;
class ModuleD3D12;
class ModuleCamera;
class ModuleRender;
class ModuleResources;
class ModuleDescriptors;
class ModuleLevel;
class ModuleSamplers;

class Application
{
public:

	Application(int argc, wchar_t** argv, void* hWnd);
	~Application();

	bool         init();
	void         update();
	bool         cleanUp();

    ModuleD3D12*       getD3D12() { return d3d12; }
    ModuleCamera*      getCamera() { return camera;  }
    ModuleRender*      getRender() { return render;  }
    ModuleResources*   getResources() { return resources;  }
    ModuleDescriptors* getDescriptors() { return descriptors;  }
    ModuleSamplers*    getSamplers() { return samplers;  }

    float              getFPS() const { return 1000.0f * float(MAX_FPS_TICKS) / tickSum; }
    float              getAvgElapsedMs() const { return tickSum / float(MAX_FPS_TICKS); }
    uint64_t           getElapsedMilis() const { return elapsedMilis; }

private:
    enum { MAX_FPS_TICKS = 30 };
    typedef std::array<uint64_t, MAX_FPS_TICKS> TickList;

    std::vector<Module*> modules;

    ModuleD3D12* d3d12 = nullptr;
    ModuleCamera* camera = nullptr;
    ModuleRender* render = nullptr;
    ModuleResources* resources = nullptr;
    ModuleDescriptors* descriptors = nullptr;
    ModuleSamplers* samplers = nullptr;
    ModuleLevel* level = nullptr;

    uint64_t  lastMilis = 0;
    TickList  tickList;
    uint64_t  tickIndex;
    uint64_t  tickSum = 0;
    uint64_t  elapsedMilis = 0;
};

extern Application* app;