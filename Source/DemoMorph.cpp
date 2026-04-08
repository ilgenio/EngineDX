#include "Globals.h"

#include "DemoMorph.h"

#include "Application.h"
#include "ModuleCamera.h"
#include "ModuleScene.h"
#include "ModuleRender.h"

#include "Scene.h"
#include "Model.h"
#include "Skybox.h"
#include "AnimationClip.h"

bool DemoMorph::init() 
{
    ModuleScene* scene = app->getScene();

    app->getScene()->getSkybox()->init("Assets/Textures/san_giuseppe_bridge_4k.hdr", false);

    modelIdx = scene->addModel("Assets/Models/MorphStressTest/MorphStressTest.gltf", "Assets/Models/MorphStressTest/");

    for (UINT i = 0; i < CLIP_COUNT; ++i)
    {
        clips[i] = scene->addClip("Assets/Models/MorphStressTest/MorphStressTest.gltf", i);
    }

    active = 0;
    scene->getModel(modelIdx)->playAnim(scene->getClip(active));

    ModuleCamera* camera = app->getCamera();

    camera->setPolar(XMConvertToRadians(1.30f));
    camera->setAzimuthal(XMConvertToRadians(-11.61f));
    camera->setTranslation(Vector3(0.0f, 1.24f, 4.65f));

    return true;
}

void DemoMorph::preRender()
{
    ImGui::Begin("Viewer Options");
    ImGui::Separator();
    if (ImGui::Combo("Animation", (int*)&active, "Individual\0Wave\0Pulse\0"))
    {
        ModuleScene* scene = app->getScene();

        scene->getModel(modelIdx)->playAnim(scene->getClip(active));
    }
    ImGui::End();
}


