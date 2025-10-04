#pragma once

#include "Module.h"
#include <span>
#include <functional>

struct DemoDescriptors
{
    const char* name;
    const char* description;
    Module* (*createFunc) ();
};


std::span<DemoDescriptors> getDemoDescriptors();