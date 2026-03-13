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

    app->getScene()->getSkybox()->init("Assets/Textures/san_giuseppe_bridge_4k.hdr", false);

    UINT modelIdx = scene->addModel("Assets/Models/BistroExterior/BistroExterior.gltf", "Assets/Models/BistroExterior/");
   
    ModuleCamera* camera = app->getCamera();

    camera->setPolar(XMConvertToRadians(1.30f));
    camera->setAzimuthal(XMConvertToRadians(-11.61f));
    camera->setTranslation(Vector3(0.0f, 1.24f, 4.65f));

    return true;
}



