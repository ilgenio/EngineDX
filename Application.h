#pragma once

#include "Globals.h"


#include <vector>

class Module;
class ModuleRender;

class Application
{
public:

	Application(int argc, wchar_t** argv, void* hWnd);
	~Application();

	bool         init();
	UpdateStatus update();
	bool         cleanUp();

    ModuleRender* getRender() { return render; }

private:

    std::vector<Module*> modules;

    ModuleRender *render = nullptr;
};

extern Application* app;