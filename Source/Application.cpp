#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleD3D12.h"
#include "ModuleInput.h"
#include "ModuleCamera.h"
#include "ModuleRender.h"
#include "ModuleResources.h"
#include "ModuleDescriptors.h"
#include "ModuleSamplers.h"
#include "ModuleLevel.h"

#include "Exercise1.h"
#include "Exercise2.h"
#include "Exercise3.h"
#include "Exercise4.h"

Application::Application(int argc, wchar_t** argv, void* hWnd)
{
    modules.push_back(d3d12 = new ModuleD3D12((HWND)hWnd));
    modules.push_back(new ModuleInput((HWND)hWnd));
    modules.push_back(camera = new ModuleCamera());
    modules.push_back(resources = new ModuleResources());
    modules.push_back(descriptors = new ModuleDescriptors());
    modules.push_back(samplers = new ModuleSamplers());

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
	bool ret = true;

	for(auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
		ret = (*it)->cleanUp();

	return ret;
}
