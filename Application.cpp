#include "Application.h"
#include "ModuleRender.h"
#include "ModuleD3D12.h"
#include "ModuleInput.h"
#include "ModuleCamera.h"

#include "Exercise1.h"
#include "Exercise2.h"
#include "Exercise3.h"

Application::Application(int argc, wchar_t** argv, void* hWnd)
{
     modules.push_back(d3d12 = new ModuleD3D12((HWND)hWnd));
     modules.push_back(new ModuleInput());
     modules.push_back(camera = new ModuleCamera());

    if(argc > 1)
    {
        if(wcscmp(argv[1], L"Exercise1") == 0)
        {
            modules.push_back(new Exercise1);
        }
        else if(wcscmp(argv[1], L"Exercise2") == 0)
        {
            modules.push_back(new Exercise2);
        }
        else if(wcscmp(argv[1], L"Exercise3") == 0)
        {
            modules.push_back(new Exercise3);
        }
    }
}

Application::~Application()
{
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

UpdateStatus Application::update()
{
	UpdateStatus ret = UPDATE_CONTINUE;

    // Update milis
    //uint32_t currentMilis = SDL_GetTicks();
    //elapsedMilis = currentMilis - lastMilis;
    //lastMilis = currentMilis;
    //tickSum -= tickList[tickIndex];
    //tickSum += elapsedMilis;
    //tickList[tickIndex] = elapsedMilis;
    //tickIndex = (tickIndex + 1) % MAX_FPS_TICKS;

	for(auto it = modules.begin(); it != modules.end() && ret == UPDATE_CONTINUE; ++it)
		ret = (*it)->preUpdate();

	for(auto it = modules.begin(); it != modules.end() && ret == UPDATE_CONTINUE; ++it)
		ret = (*it)->update();

	for(auto it = modules.begin(); it != modules.end() && ret == UPDATE_CONTINUE; ++it)
		ret = (*it)->postUpdate();

	return ret;
}

bool Application::cleanUp()
{
	bool ret = true;

    //if (vulkan) vulkan->SyncCleanUp();

	for(auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
		ret = (*it)->cleanUp();

	return ret;
}
