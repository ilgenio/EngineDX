#include "Globals.h"

#include "DemoScene.h"

#include "Application.h"
#include "ModuleCamera.h"
#include "ModuleScene.h"
#include "ModuleRender.h"

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
   
    ModuleCamera* camera = app->getCamera();
    ModuleRender* render = app->getRender();
 
    std::string configStr = readFile(std::string("scene_config.json"));
    if(!configStr.empty())
    {
        json::jobject config = json::jobject::parse(configStr);
        json::jobject cameraObj = config["camera"]; 
        json::jobject renderObj = config["render"];
        camera->deserialize(cameraObj);
        render->deserialize(renderObj);
    }
    else
    {
        camera->setPolar(XMConvertToRadians(-90.0f));
        camera->setAzimuthal(XMConvertToRadians(-15.0f));
        camera->setTranslation(Vector3(-6.0f, 4.0f, -0.5f));
    }

    return true;
}

bool DemoScene::cleanUp()
{
    ModuleCamera* camera = app->getCamera();
    ModuleRender* render = app->getRender();

    json::jobject cameraObj;
    camera->serialize(cameraObj);

    json::jobject renderObj;
    render->serialize(renderObj);


    json::jobject config;
    config["camera"] = cameraObj;
    config["render"] = renderObj;

    std::string configStr = config;
    writeFile("scene_config.json", configStr);

    return true;
}

