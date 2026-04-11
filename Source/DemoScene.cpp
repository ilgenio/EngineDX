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
    if (!deserialize())
    {
        ModuleScene* scene = app->getScene();

        scene->getSkybox()->init("Assets/Textures/qwantani_very_dark.hdr", false);

        scene->addModel("Assets/Models/Sponza/Sponza.gltf");

        ModuleCamera* camera = app->getCamera();
        camera->setPolar(XMConvertToRadians(-90.0f));
        camera->setAzimuthal(XMConvertToRadians(-15.0f));
        camera->setTranslation(Vector3(-6.0f, 4.0f, -0.5f));

        Vector3 lightDir = Vector3(-0.5f, -1.0f, -0.5f);
        lightDir.Normalize();

        scene->addLight(Spot(
            lightDir,           // Direction
            10.0f,              // Radius
            Vector3(0.0f, 1.0f, 0.0f), // Position
            0.75f,               // Inner cone angle (cosine)
            0.7f,              // Outer cone angle (cosine)
            Color(1.0f, 1.0f, 1.0f, 5.0f) // Color (white)
        ));

        scene->addLight(Point(
            Vector3(2.0f, 1.0f, 0.0f), // Position
            2.0f,              // Radius
            Color(1.0f, 1.0f, 1.0f, 5.0f) // Color (white)
        ));
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





