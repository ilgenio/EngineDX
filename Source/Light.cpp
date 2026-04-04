#include "Globals.h"

#include "Light.h"
#include "Scene.h"

Light::~Light()
{
    if (scene)
    {
        scene->onRemoveLight(this);
    }
}