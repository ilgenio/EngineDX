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
    
    if (ImGui::Checkbox("Show T-Pose", &showTPose))
    {
        ModuleScene* scene = app->getScene();
        std::shared_ptr<Model> character = scene->getModel(modelIdx);

        if (showTPose)
        {
            character->PlayAnim(scene->getClip(anims[TPOSE]));
            currentAnim = TPOSE;
        }
        else
        {
            character->PlayAnim(scene->getClip(anims[IDLE_LONG]));
            currentAnim = IDLE_LONG;
        }
    }

    if (ImGui::Button("Reset"))
    {
        ModuleScene* scene = app->getScene();
        std::shared_ptr<Model> character = scene->getModel(modelIdx);

        character->setRootTransform(Matrix::Identity);
    }

    ImGui::InputFloat("Linear Speed", &linearSpeed);


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
            setAnimation(RUN);
        }

        if (keyState.Left)
        {
            moveCharacter(localLeft);
        }
        if (keyState.Right)
        {
            moveCharacter(localRight);
        }
        else
        {
            moveCharacter(localForward);
        }
    }   
    else
    {
        if (currentAnim == RUN)
        {
            setAnimation(IDLE_LONG);
        }
    }
}

void DemoSkinning::setAnimation(Anims anim)
{
    ModuleScene* scene = app->getScene();
    std::shared_ptr<Model> character = scene->getModel(modelIdx);
    character->PlayAnim(scene->getClip(anims[anim]));
    currentAnim = anim;
}

void DemoSkinning::moveCharacter(const Vector3& localDir)
{
    ModuleScene* scene = app->getScene();
    std::shared_ptr<Model> character = scene->getModel(modelIdx);
    float elapsedSec = app->getElapsedMilis() * 0.001f;

    Matrix transform = character->getRootTransform();

    float angleDiff = atan2f(localDir.x, localDir.z) - atan2f(localForward.x, localForward.z) ;
    while (angleDiff < -M_PI) angleDiff += M_PI * 2.0f;
    while (angleDiff > M_PI) angleDiff -= M_PI * 2.0f;

    float maxAngle = angularSpeed * elapsedSec;
    float angle = angleDiff > 0.0f ? std::min(maxAngle, angleDiff) : std::max(-maxAngle, angleDiff);

    Quaternion rot = Quaternion::CreateFromAxisAngle(Vector3::UnitY, angle);
    Matrix rotMat = Matrix::CreateFromQuaternion(rot);
    transform = rotMat * transform;

    Vector3 forward = Vector3::TransformNormal(localForward, transform);

    Vector3 currentPos = transform.Translation();
    Vector3 targetPos = currentPos + forward * linearSpeed * elapsedSec;
    transform.Translation(targetPos);

    character->setRootTransform(transform);
}

