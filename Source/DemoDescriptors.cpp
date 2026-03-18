#include "Globals.h"

#include "DemoDescriptors.h"

#include "DemoAnimation.h"
#include "DemoSkinning.h"
#include "DemoScene.h"
#include "DemoMorph.h"
#include "DemoTrail.h"
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
    { "DemoMorphTrail", "Shows trails", false, []() -> Module* { return new DemoTrail(); } },
    { "DemoMorphTargets", "Shows morph targets", false, []() -> Module* { return new DemoMorph(); } },
    { "DemoCharacter", "Shows Skinning and character movement", false, []() -> Module* { return new DemoSkinning(); } },
    { "DemoAnimation", "Shows Animations working (no skinning)", false, []() -> Module* { return new DemoAnimation(); } },
    { "DemoScene", "Shows a demo scene with various elements", false, []() -> Module* { return new DemoScene(); } },
    { "Exercise1", "Clears the screen with a solid color", true, []() -> Module* { return new Exercise1(); } },
    { "Exercise2", "Renders a 2D triangle", true, []() -> Module* { return new Exercise2(); } },
    { "Exercise3", "Renders a 3D triangle", true, []() -> Module* { return new Exercise3(); } },
    { "Exercise4", "Renders a quad texture", true, []() -> Module* { return new Exercise4(); } },
    { "Exercise5", "Renders a model with a colour texture", true, []() -> Module* { return new Exercise5(); } },
    { "Exercise6", "Renders a model using Phong", true, []() -> Module* { return new Exercise6(); } },
    { "Exercise7", "Render a model using PBR Phong (render to texture)", true, []() -> Module* { return new Exercise7(); } },
    { "Exercise8", "Renders a model lit  with different light sources, tonemapping and gamma correction", true, []() -> Module* { return new Exercise8(); } },
    { "Exercise9", "Renders a Cubemap skybox", true, []() -> Module* { return new Exercise9(); } },
    { "Exercise10", "Render a model using Cook Torrance BRDF", true, []() -> Module* { return new Exercise10(); } },
    { "Exercise11", "Renders a sphere using Image Based Lighting irradiance", true, []() -> Module* { return new Exercise11(); } },
    { "Exercise12", "Renders a model to test Image Based Lighting irradiance and radiance", true, []() -> Module* { return new Exercise12(); } },
    { "Exercise13", "Renders 3 models to test normal maps, ambient occlusion and emissive", true, []() -> Module* { return new Exercise13(); } },
};

std::span<DemoDescriptors> getDemoDescriptors()
{
    return std::span<DemoDescriptors>(demos, sizeof(demos) / sizeof(DemoDescriptors));  
}