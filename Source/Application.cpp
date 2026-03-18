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
#include "ModuleScene.h"
#include "ModuleRender.h"
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

    bool demoFound = false;
    if (argc > 1)
    {
        char cstr[512];
        size_t converted = 0;
        wcstombs_s(&converted, cstr, 512, argv[1], 511);
        auto demos = getDemoDescriptors();
        for (UINT i = 0; !demoFound && i < demos.size(); ++i)
        {
            if (strcmp(cstr, demos[i].name) == 0)
            {
                setDemo(i);
                demoFound = true;
            }
        }
    }


    if(!demoFound)
    {
        modules.push_back(startMenu = new StartMenu);
    }
}

void Application::setDemo(UINT index)
{
    swapDemo = index;
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
            swapDemoIfNeeded();

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

void Application::swapDemoIfNeeded()
{
    auto demos = getDemoDescriptors();

    if (swapDemo < demos.size())
    {
        const auto& demoDesc = demos[swapDemo];

        auto releaseModule = [&](Module* module)
            {
                if (module)
                {
                    auto it = std::find(modules.begin(), modules.end(), module);
                    if (it != modules.end())
                    {
                        modules.erase(it);
                    }

                    module->cleanUp();
                    delete module;
                }
            };

        if (demoDesc.isExercise)
        {
            releaseModule(render);
            releaseModule(scene);
            releaseModule(demo);
            releaseModule(startMenu);

            modules.push_back(demo = demoDesc.createFunc());
            demo->init();
        }
        else
        {
            releaseModule(demo);
            releaseModule(startMenu);

            if (scene == nullptr)
            {
                modules.push_back(scene = new ModuleScene);
                scene->init();
            }

            if (render == nullptr)
            {
                modules.push_back(render = new ModuleRender);
                render->init();
            }

            modules.push_back(demo = demoDesc.createFunc());
            demo->init();
        }

        swapDemo = UINT_MAX;
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
