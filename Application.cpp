#include "Application.h"
#include "ModuleRender.h"

Application::Application(int argc, char** argv, void* hWnd)
{
    modules.push_back(render = new ModuleRender((HWND)hWnd));
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

	for(auto it = modules.rbegin(); it != modules.rend() && ret; ++it)
		ret = (*it)->cleanUp();

	return ret;
}
