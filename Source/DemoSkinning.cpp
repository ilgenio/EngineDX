#include "Globals.h"

#include "DemoSkinning.h"

#include "Application.h"
#include "ModuleCamera.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "Model.h"
#include "Skybox.h"
#include "AnimationClip.h"
#include "ModuleRender.h"

bool DemoSkinning::init() 
{
    ModuleScene* scene = app->getScene();

    app->getScene()->getSkybox()->init("Assets/Textures/san_giuseppe_bridge_4k.hdr", false);

    UINT modelIdx = scene->addModel("Assets/Models/KyleRobot/KyleRobot.gltf", "Assets/Models/KyleRobot/");
    UINT animIdx = scene->addClip("Assets/Models/KyleRobot/Stand_Idle/Stand_idle.gltf", 0);
    scene->getModel(modelIdx)->PlayAnim(scene->getClip(animIdx));

    ModuleRender* render = app->getRender();
    render->addDebugDrawModel(modelIdx);

    ModuleCamera* camera = app->getCamera();

    camera->setPolar(XMConvertToRadians(1.30f));
    camera->setAzimuthal(XMConvertToRadians(-11.61f));
    camera->setTranslation(Vector3(0.0f, 1.24f, 4.65f));

    return true;
}
