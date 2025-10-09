#include "Globals.h"

#include "DemoDescriptors.h"

#include "Demo.h"
#include "Exercises/Exercise1.h"
#include "Exercises/Exercise2.h"
#include "Exercises/Exercise3.h"
#include "Exercises/Exercise4.h"
#include "Exercises/Exercise5.h"
#include "Exercises/Exercise6.h"
#include "Exercises/Exercise7.h"
#include "Exercises/Exercise8.h"
#include "Exercises/Exercise9.h"
#include "Exercises/Exercise10.h"
#include "Exercises/Exercise11.h"
#include "Exercises/Exercise12.h"
#include "Exercises/Exercise13.h"

DemoDescriptors demos[] = {
    { "Demo", "Shows Rendering features", []() -> Module* { return new Demo(); } },
    { "Exercise1", "Clears the screen with a solid color", []() -> Module* { return new Exercise1(); } },
    { "Exercise2", "Renders a 2D triangle", []() -> Module* { return new Exercise2(); } },
    { "Exercise3", "Renders a 3D triangle", []() -> Module* { return new Exercise3(); } },
    { "Exercise4", "Renders a quad texture", []() -> Module* { return new Exercise4(); } },
    { "Exercise5", "Renders a model with a colour texture", []() -> Module* { return new Exercise5(); } },
    { "Exercise6", "Renders a model using Phong", []() -> Module* { return new Exercise6(); } },
    { "Exercise7", "Render a model using PBR Phong (render to texture)", []() -> Module* { return new Exercise7(); } },
    { "Exercise8", "Renders a model lit  with different light sources, tonemapping and gamma correction", []() -> Module* { return new Exercise8(); } },
    { "Exercise9", "Renders a Cubemap skybox", []() -> Module* { return new Exercise9(); } },
    { "Exercise10", "Render a model using Cook Torrance BRDF", []() -> Module* { return new Exercise10(); } },
    { "Exercise11", "Renders a sphere using Image Based Lighting irradiance", []() -> Module* { return new Exercise11(); } },
    { "Exercise12", "Renders a model to test Image Based Lighting irradiance and radiance", []() -> Module* { return new Exercise12(); } },
    { "Exercise13", "Renders a model to test ambient occlusion", []() -> Module* { return new Exercise13(); } },
};

std::span<DemoDescriptors> getDemoDescriptors()
{
    return std::span<DemoDescriptors>(demos, sizeof(demos) / sizeof(DemoDescriptors));  
}