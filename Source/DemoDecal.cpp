#include "Globals.h"

#include "DemoDecal.h"

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


bool DemoDecal::init() 
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

        Vector3 lightDir = Vector3(-0.5f, -1.0f, -0.5f);
        lightDir.Normalize();

        for(int x = -4; x <= 4; x += 2)
        {
             for(int z = -4; z <= 4; z += 2)
             {
                 for (int y = 1; y <= 4; y += 2)
                 {
                     scene->addLight(Point(
                         Vector3(float(x), float(y), float(z)), // Position
                         2.0f,              // Radius
                         Color(1.0f, 1.0f, 1.0f, 10.0f) // Color (white)
                     ));
                 }
             }
        }

        scene->addDecal(
            "Assets/Decals/diffuse_green_single.tga",
            "Assets/Decals/normal_single.tga",
            Matrix::Identity);

    }

    return true;
}

bool DemoDecal::cleanUp()
{
    serialize();

    return true;
}

void DemoDecal::serialize()
{
    ModuleSceneEditor* sceneEditor = app->getSceneEditor();

    Json config = sceneEditor->serialize();

    std::string configStr;
    config.dump(configStr);

    writeFile("scene_config.json", configStr);
}

bool DemoDecal::deserialize()
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





