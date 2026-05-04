#include "Globals.h"

#include "DemoScene.h"

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


bool DemoScene::init() 
{
    ModuleScene* scene = app->getScene();

    if (!deserialize())
    {
        scene->getSkybox()->init("Assets/Textures/qwantani_very_dark.hdr", false);

        scene->addModel("Assets/Models/Sponza/Sponza.gltf");

        ModuleCamera* camera = app->getCamera();
        camera->setPolar(XMConvertToRadians(-90.0f));
        camera->setAzimuthal(XMConvertToRadians(-15.0f));
        camera->setTranslation(Vector3(-6.0f, 4.0f, -0.5f));

        scene->addLight(Directional(Vector3(-0.5f, -1.0f, -0.5f), Color(1.0f, 1.0f, 1.0f, 1.0f)));
    }

    return true;
}

bool DemoScene::cleanUp()
{
    serialize();

    return true;
}

void DemoScene::serialize()
{
    ModuleSceneEditor* sceneEditor = app->getSceneEditor();

    Json config = sceneEditor->serialize();

    std::string configStr;
    config.dump(configStr);

    writeFile("scene_config.json", configStr);
}

bool DemoScene::deserialize()
{
    ModuleSceneEditor* sceneEditor = app->getSceneEditor();

    std::string configStr = readFile(std::string("scene_config.json"));
    std::string error;
    Json configJson = Json::parse(configStr, error);

    if (error.empty())
    {
        sceneEditor->deserialize(configJson);

        return true;
    }

    return false;
}





