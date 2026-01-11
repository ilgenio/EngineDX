#include "Globals.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleInput.h"
#include "ModuleCamera.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleTargetDescriptors.h"
#include "ModuleSamplers.h"
#include "ModuleRingBuffer.h"
#include "ModuleStaticBuffer.h"
#include "DemoDescriptors.h"

#include "StartMenu.h"  

Application::Application(int argc, wchar_t** argv, void* hWnd)
{
    modules.push_back(d3d12 = new ModuleD3D12((HWND)hWnd));
    modules.push_back(new ModuleInput((HWND)hWnd));
    modules.push_back(camera = new ModuleCamera());
    modules.push_back(resources = new ModuleResources());
    modules.push_back(shaderDescriptors = new ModuleShaderDescriptors());
    modules.push_back(targetDescriptors = new ModuleTargetDescriptors());
    modules.push_back(samplers = new ModuleSamplers());
    modules.push_back(ringBuffer = new ModuleRingBuffer());
    modules.push_back(staticBuffer = new ModuleStaticBuffer());

    if (argc > 1)
    {
        char cstr[512];
        size_t converted = 0;
        wcstombs_s(&converted, cstr, 512, argv[1], 511);
        for(auto demo : getDemoDescriptors())
            if (strcmp(cstr, demo.name) == 0)
            {
                modules.push_back(demo.createFunc());
                break;
            }
    }
    else
    {
        modules.push_back(new StartMenu);
    }
}

Application::~Application()
{
    cleanUp();

	for(auto it = modules.rbegin(); it != modules.rend(); ++it)
    {
        delete *it;
    }
}
 
bool Application::init()
{
	bool ret = true;

	for(auto it = modules.begin(); it != modules.end() && ret; ++it)
		ret = (*it)->init();

    lastMilis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	return ret;
}

void Application::update()
{
    using namespace std::chrono_literals;

    if (!updating)
    {
        updating = true;

        // Update milis
        uint64_t currentMilis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        elapsedMilis = currentMilis - lastMilis;
        lastMilis = currentMilis;
        tickSum -= tickList[tickIndex];
        tickSum += elapsedMilis;
        tickList[tickIndex] = elapsedMilis;
        tickIndex = (tickIndex + 1) % MAX_FPS_TICKS;

        if (!app->paused)
        {
            for (auto it = swapModules.begin(); it != swapModules.end(); ++it)
            {
                auto pos = std::find(modules.begin(), modules.end(), it->first);
                if (pos != modules.end())
                {
                    (*pos)->cleanUp();
                    delete* pos;

                    it->second->init();
                    *pos = it->second;
                }
            }

            swapModules.clear();

            for (auto it = modules.begin(); it != modules.end(); ++it)
                (*it)->update();

            for (auto it = modules.begin(); it != modules.end(); ++it)
                (*it)->preRender();

            for (auto it = modules.begin(); it != modules.end(); ++it)
                (*it)->render();

            for (auto it = modules.begin(); it != modules.end(); ++it)
                (*it)->postRender();
        }

        updating = false;
    }
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
