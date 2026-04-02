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

bool DemoScene::init() 
{
    ModuleScene* scene = app->getScene();

    app->getScene()->getSkybox()->init("Assets/Textures/qwantani_moon_noon_puresky_4k.hdr", false);

    UINT modelIdx = scene->addModel("Assets/Models/Sponza/Sponza.gltf", "Assets/Models/Sponza/");
   
    ModuleCamera* camera = app->getCamera();

    camera->setPolar(XMConvertToRadians(-90.0f));
    camera->setAzimuthal(XMConvertToRadians(-15.0f));
    camera->setTranslation(Vector3(-6.0f, 4.0f, -0.5f));

    return true;
}



