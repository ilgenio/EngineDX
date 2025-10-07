#pragma once

#include <span>

class Module;

struct DemoDescriptors
{
    const char* name;                                      // Name of the demo or module
    const char* description;                               // Short description of the demo
    Module* (*createFunc) ();                              // Factory function to create the module instance
};

// Returns a span of available demo descriptors for module discovery and selection.
// Use this function to enumerate all demos that can be launched in the application.
std::span<DemoDescriptors> getDemoDescriptors();