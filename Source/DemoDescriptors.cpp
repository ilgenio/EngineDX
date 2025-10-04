#include "Globals.h"

#include "DemoDescriptors.h"

#include "Demo.h"
#include "Exercise1.h"
#include "Exercise2.h"
#include "Exercise3.h"
#include "Exercise4.h"
#include "Exercise5.h"
#include "Exercise6.h"
#include "Exercise7.h"
#include "Exercise8.h"
#include "Exercise9.h"
#include "Exercise10.h"
#include "Exercise11.h"
#include "Exercise12.h"

DemoDescriptors demos[] = {
    { "Demo", "Shows Rendering features", []() -> Module* { return new Demo(); } },
    { "Exercise1", "Clears the screen with a solid color", []() -> Module* { return new Exercise1(); } },
    { "Exercise2", "Renders a 2D triangle", []() -> Module* { return new Exercise2(); } },
    { "Exercise3", "Renders a 3D triangle", []() -> Module* { return new Exercise3(); } },
    { "Exercise4", "Renders a quad texture", []() -> Module* { return new Exercise4(); } },
    { "Exercise5", "Renders a model with a colour texture", []() -> Module* { return new Exercise5(); } },
    { "Exercise6", "Renders a model using Phong", []() -> Module* { return new Exercise6(); } },
    { "Exercise7", "Renders a model using PBR Phong", []() -> Module* { return new Exercise7(); } },
    { "Exercise8", "Renders a model lit  with different light sources, tonemapping and gamma correction", []() -> Module* { return new Exercise8(); } },
    { "Exercise9", "Renders a Cubemap skybox", []() -> Module* { return new Exercise9(); } },
    { "Exercise10", "Render a model using Cook Torrance BRDF", []() -> Module* { return new Exercise10(); } },
    { "Exercise11", "Renders a sphere using Image Based Lighting irradiance", []() -> Module* { return new Exercise11(); } },
    { "Exercise12", "Renders a model to test Image Based Lighting irradiance and radiance", []() -> Module* { return new Exercise12(); } },
};

std::span<DemoDescriptors> getDemoDescriptors()
{
    return std::span<DemoDescriptors>(demos, sizeof(demos) / sizeof(DemoDescriptors));  
}