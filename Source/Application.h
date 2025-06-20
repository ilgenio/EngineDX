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
class ModuleShaderDescriptors;
class ModuleRTDescriptors;
class ModuleDSDescriptors;
class ModuleLevel;
class ModuleSamplers;
class ModuleRingBuffer;

class Application
{
public:

	Application(int argc, wchar_t** argv, void* hWnd);
	~Application();

	bool         init();
	void         update();
	bool         cleanUp();

    ModuleD3D12*                getD3D12() { return d3d12; }
    ModuleCamera*               getCamera() { return camera;  }
    ModuleRender*               getRender() { return render;  }
    ModuleResources*            getResources() { return resources;  }
    ModuleShaderDescriptors*    getShaderDescriptors() { return shaderDescriptors;  }
    ModuleRTDescriptors*        getRTDescriptors() { return rtDescriptors;  }
    ModuleDSDescriptors*        getDSDescriptors() { return dsDescriptors; }
    ModuleSamplers*             getSamplers() { return samplers;  }
    ModuleRingBuffer*           getRingBuffer() { return ringBuffer; }

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
    ModuleShaderDescriptors* shaderDescriptors = nullptr;
    ModuleRTDescriptors* rtDescriptors = nullptr;
    ModuleDSDescriptors* dsDescriptors = nullptr;
    ModuleSamplers* samplers = nullptr;
    ModuleRingBuffer* ringBuffer = nullptr;
    ModuleLevel* level = nullptr;

    uint64_t  lastMilis = 0;
    TickList  tickList;
    uint64_t  tickIndex;
    uint64_t  tickSum = 0;
    uint64_t  elapsedMilis = 0;
};

extern Application* app;
