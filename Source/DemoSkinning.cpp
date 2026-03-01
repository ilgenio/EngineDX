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

#include "Keyboard.h"

bool DemoSkinning::init() 
{
    ModuleScene* scene = app->getScene();

    app->getScene()->getSkybox()->init("Assets/Textures/san_giuseppe_bridge_4k.hdr", false);

    modelIdx = scene->addModel("Assets/Models/Elf_arbalester/Elf_arbalester.gltf", "Assets/Models/Elf_arbalester/");

    for (int i = 0; i < ANIM_COUNT; ++i)
    {
        anims[i] = scene->addClip("Assets/Models/Elf_arbalester/Elf_arbalester.gltf", i);
    }

    std::shared_ptr<Model> character = scene->getModel(modelIdx);
    character->PlayAnim(scene->getClip(anims[IDLE_LONG]));
    currentAnim = IDLE_LONG;

    ModuleRender* render = app->getRender();
    render->addDebugDrawModel(modelIdx);

    ModuleCamera* camera = app->getCamera();

    camera->setPolar(XMConvertToRadians(1.30f));
    camera->setAzimuthal(XMConvertToRadians(-11.61f));
    camera->setTranslation(Vector3(0.0f, 1.24f, 4.65f));

    return true;
}   

void DemoSkinning::preRender()
{
    ImGui::Begin("Demo Viewer Options");
        

    ImGui::End();
}

void DemoSkinning::update()
{
    Keyboard& keyboard = Keyboard::Get();

    const Keyboard::State& keyState = keyboard.GetState();

    if (keyState.Up)
    {
        if (currentAnim == IDLE_LONG)
        {
            ModuleScene* scene = app->getScene();
            std::shared_ptr<Model> character = scene->getModel(modelIdx);
            character->PlayAnim(scene->getClip(anims[RUN]));
            currentAnim = RUN;
        }
    }
    else
    {
        if (currentAnim == RUN)
        {
            ModuleScene* scene = app->getScene();
            std::shared_ptr<Model> character = scene->getModel(modelIdx);
            character->PlayAnim(scene->getClip(anims[IDLE_LONG]));
            currentAnim = IDLE_LONG;
        }
    }


}

