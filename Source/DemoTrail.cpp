#include "Globals.h"

#include "DemoTrail.h"

#include "Application.h"
#include "ModuleCamera.h"
#include "ModuleScene.h"
#include "ModuleRender.h"

#include "Scene.h"
#include "Model.h"
#include "Skybox.h"
#include "AnimationClip.h"

bool DemoTrail::init() 
{
    ModuleScene* scene = app->getScene();

    app->getScene()->getSkybox()->init("Assets/Textures/san_giuseppe_bridge_4k.hdr", false);

    UINT modelIdx = scene->addModel("Assets/Models/Sword/sword.gltf", "Assets/Models/Sword/");
    UINT animIdx = scene->addClip("Assets/Models/Sword/sword.gltf", 0);

    scene->getModel(modelIdx)->playAnim(scene->getClip(animIdx));

    app->getRender()->addDebugDrawModel(modelIdx);

    ModuleCamera* camera = app->getCamera();

    camera->setPolar(XMConvertToRadians(1.30f));
    camera->setAzimuthal(XMConvertToRadians(-11.61f));
    camera->setTranslation(Vector3(0.0f, 1.24f, 4.65f));

    return true;
}



