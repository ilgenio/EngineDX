#include "Globals.h"

#include "DemoScene.h"

#include "Application.h"
#include "ModuleCamera.h"
#include "ModuleScene.h"
#include "ModuleRender.h"
#include "Light.h"

#include "Scene.h"
#include "Model.h"
#include "Skybox.h"
#include "AnimationClip.h"

#include "json_utils.h"


bool DemoScene::init() 
{
    ModuleScene* scene = app->getScene();

    app->getScene()->getSkybox()->init("Assets/Textures/qwantani_very_dark.hdr", false);

    UINT modelIdx = scene->addModel("Assets/Models/Sponza/Sponza.gltf", "Assets/Models/Sponza/");
   
    if (!deserialize())
    {
        ModuleCamera* camera = app->getCamera();
        camera->setPolar(XMConvertToRadians(-90.0f));
        camera->setAzimuthal(XMConvertToRadians(-15.0f));
        camera->setTranslation(Vector3(-6.0f, 4.0f, -0.5f));

    }

    addLights();

    return true;
}

bool DemoScene::cleanUp()
{
    serialize();

    return true;
}

void DemoScene::serialize()
{
    ModuleCamera* camera = app->getCamera();
    ModuleRender* render = app->getRender();

    Json cameraObj;
    camera->serialize(cameraObj);

    Json renderObj;
    render->serialize(renderObj);

    Json::object config;

    config["camera"] = cameraObj;
    config["render"] = renderObj;

    std::string configStr;
    Json(config).dump(configStr);


    writeFile("scene_config.json", configStr);
}

bool DemoScene::deserialize()
{
    ModuleCamera* camera = app->getCamera();
    ModuleRender* render = app->getRender();

    std::string configStr = readFile(std::string("scene_config.json"));
    std::string error;
    Json configJson = Json::parse(configStr, error);
    if (error.empty())
    {
        const Json& cameraJson = configJson["camera"];
        const Json& renderJson = configJson["render"];

        camera->deserialize(cameraJson);
        render->deserialize(renderJson);

        return true;
    }

    return false;
}

void DemoScene::addLights()
{
    ModuleScene* scene = app->getScene();

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




