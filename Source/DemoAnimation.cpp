#include "Globals.h"

#include "DemoAnimation.h"

#include "Application.h"
#include "ModuleCamera.h"
#include "ModuleScene.h"

#include "Scene.h"
#include "Model.h"
#include "Skybox.h"
#include "AnimationClip.h"

bool DemoAnimation::init() 
{
    ModuleScene* scene = app->getScene();

    app->getScene()->getSkybox()->init("Assets/Textures/san_giuseppe_bridge_4k.hdr", false);

    UINT modelIdx = scene->addModel("Assets/Models/busterDrone/busterDrone.gltf", "Assets/Models/busterDrone");
    UINT animIdx = scene->addClip("Assets/Models/busterDrone/busterDrone.gltf", 0);
    scene->getModel(modelIdx)->PlayAnim(scene->getClip(animIdx));

    ModuleCamera* camera = app->getCamera();

    camera->setPolar(XMConvertToRadians(1.30f));
    camera->setAzimuthal(XMConvertToRadians(-11.61f));
    camera->setTranslation(Vector3(0.0f, 1.24f, 4.65f));

    return true;
}
