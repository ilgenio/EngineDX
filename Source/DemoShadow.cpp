#include "Globals.h"

#include "DemoShadow.h"

#include "Application.h"
#include "ModuleCamera.h"
#include "ModuleScene.h"
#include "ModuleRender.h"
#include "ModuleSceneEditor.h"
#include "Light.h"

#include "Scene.h"
#include "Model.h"
#include "Skybox.h"
#include "AnimationClip.h"

#include "json_utils.h"


bool DemoShadow::init() 
{
    ModuleScene* scene = app->getScene();

    if (!deserialize())
    {
        scene->getSkybox()->init("Assets/Textures/qwantani_very_dark.hdr", false);

        UINT modelIdx = scene->addModel("Assets/Models/busterDrone/busterDrone.gltf");

        scene->addLight(Directional(Vector3(-0.15f, -0.8f, -0.6f), Color(1.0f, 1.0f, 1.0f, 3.5f)));
    }

    UINT animIdx = scene->addClip("Assets/Models/busterDrone/busterDrone.gltf", 0);
    if (scene->getModelCount() > 0 && scene->getClipCount() > 0)
    {
        scene->getModel(0)->playAnim(scene->getClip(0));
    }

    return true;
}

bool DemoShadow::cleanUp()
{
    serialize();

    return true;
}

void DemoShadow::serialize()
{
    ModuleSceneEditor* sceneEditor = app->getSceneEditor();

    Json config = sceneEditor->serialize();

    std::string configStr;
    config.dump(configStr);

    writeFile("shadow_scene_config.json", configStr);
}

bool DemoShadow::deserialize()
{
    ModuleSceneEditor* sceneEditor = app->getSceneEditor();

    std::string configStr = readFile(std::string("shadow_scene_config.json"));
    std::string error;
    Json configJson = Json::parse(configStr, error);

    if (error.empty())
    {
        sceneEditor->deserialize(configJson);

        return true;
    }

    return false;
}





