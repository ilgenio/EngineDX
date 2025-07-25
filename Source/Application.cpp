#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleD3D12.h"
#include "ModuleInput.h"
#include "ModuleCamera.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleRTDescriptors.h"
#include "ModuleDSDescriptors.h"
#include "ModuleSamplers.h"
#include "ModuleRingBuffer.h"
#include "ModuleLevel.h"

#include "Exercise1.h"
#include "Exercise2.h"
#include "Exercise3.h"
#include "Exercise4.h"
#include "Exercise5.h"
#include "Exercise6.h"
#include "Exercise7.h"
#include "Exercise9.h"

Application::Application(int argc, wchar_t** argv, void* hWnd)
{
    modules.push_back(d3d12 = new ModuleD3D12((HWND)hWnd));
    modules.push_back(new ModuleInput((HWND)hWnd));
    modules.push_back(camera = new ModuleCamera());
    modules.push_back(resources = new ModuleResources());
    modules.push_back(shaderDescriptors = new ModuleShaderDescriptors());
    modules.push_back(rtDescriptors = new ModuleRTDescriptors());
    modules.push_back(dsDescriptors = new ModuleDSDescriptors());
    modules.push_back(samplers = new ModuleSamplers());
    modules.push_back(ringBuffer = new ModuleRingBuffer());

    if(argc > 1 && wcscmp(argv[1], L"Exercise1") == 0)
     {
        modules.push_back(new Exercise1);
    }
    else if(argc > 1 && wcscmp(argv[1], L"Exercise2") == 0)
    {
        modules.push_back(new Exercise2);
    }
    else if(argc > 1 && wcscmp(argv[1], L"Exercise3") == 0)
    {
        modules.push_back(new Exercise3);
    }
    else if (argc > 1 && wcscmp(argv[1], L"Exercise4") == 0)
    {
        modules.push_back(new Exercise4);
    }
    else if (argc > 1 && wcscmp(argv[1], L"Exercise5") == 0)
    {
        modules.push_back(new Exercise5);
    }
    else if (argc > 1 && wcscmp(argv[1], L"Exercise6") == 0)
    {
        modules.push_back(new Exercise6);
    }
    else if (argc > 1 && wcscmp(argv[1], L"Exercise7") == 0)
    {
        modules.push_back(new Exercise7);
    }
    else if (argc > 1 && wcscmp(argv[1], L"Exercise9") == 0)
    {
        modules.push_back(new Exercise9);
    }
    else
    {
        modules.push_back(render = new ModuleRender());
        modules.push_back(level = new ModuleLevel());
    }
}

Application::~Application()
{
    cleanUp();

	for(Module* module : modules)
    {
        delete module;
    }
}
 
bool Application::init()
{
	bool ret = true;

	for(auto it = modules.begin(); it != modules.end() && ret; ++it)
		ret = (*it)->init();

	return ret;
}

void Application::update()
{
    using namespace std::chrono_literals;

    // Update milis
    uint64_t currentMilis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    elapsedMilis = currentMilis - lastMilis;
    lastMilis = currentMilis;
    tickSum -= tickList[tickIndex];
    tickSum += elapsedMilis;
    tickList[tickIndex] = elapsedMilis;
    tickIndex = (tickIndex + 1) % MAX_FPS_TICKS;

    for (auto it = modules.begin(); it != modules.end(); ++it)
        (*it)->update();
    
    for(auto it = modules.begin(); it != modules.end(); ++it)
		(*it)->preRender();

    for (auto it = modules.begin(); it != modules.end(); ++it)
        (*it)->render();
    
    for(auto it = modules.begin(); it != modules.end(); ++it)
		(*it)->postRender();
}

bool Application::cleanUp()
{
    // needed to safely remove d3d objects 
    d3d12->flush();

	bool ret = true;

	for(auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
		ret = (*it)->cleanUp();

	return ret;
}
