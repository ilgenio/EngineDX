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
        moveCharacter();
    }   
    else
    {
        if (currentAnim == RUN)
        {
            setAnimation(IDLE_LONG);
        }
    }

    if (keyState.Left)
    {
        rotateCharacter(localLeft);
    }
    if (keyState.Right)
    {
        rotateCharacter(localRight);
    }

}

void DemoSkinning::setAnimation(Anims anim)
{
    ModuleScene* scene = app->getScene();
    std::shared_ptr<Model> character = scene->getModel(modelIdx);
    character->PlayAnim(scene->getClip(anims[anim]));
    currentAnim = anim;
}

void DemoSkinning::moveCharacter()
{
    ModuleScene* scene = app->getScene();
    std::shared_ptr<Model> character = scene->getModel(modelIdx);
    float elapsedSec = app->getElapsedMilis() * 0.001f;

    Matrix transform = character->getRootTransform();
    Vector3 translation = transform.Translation();

    Vector3 forward = Vector3::TransformNormal(localForward, transform);
    translation += forward * linearSpeed * elapsedSec;
       
    transform.Translation(translation);
    
    character->setRootTransform(transform);
}

void DemoSkinning::rotateCharacter(const Vector3& localDir)
{
    ModuleScene* scene = app->getScene();
    std::shared_ptr<Model> character = scene->getModel(modelIdx);
    float elapsedSec = app->getElapsedMilis() * 0.001f;

    float angleDiff = atan2f(localDir.x, localDir.z) - atan2f(localForward.x, localForward.z);
    while (angleDiff < -M_PI) angleDiff += M_PI * 2.0f;
    while (angleDiff > M_PI) angleDiff -= M_PI * 2.0f;

    float maxAngle = angularSpeed * elapsedSec;
    float angle = angleDiff > 0.0f ? std::min(maxAngle, angleDiff) : std::max(-maxAngle, angleDiff);

    Matrix transform = character->getRootTransform();
    Vector3 translation, scale;
    Quaternion rotation;
    transform.Decompose(scale, rotation, translation);
    
    rotation = rotation * Quaternion::CreateFromAxisAngle(Vector3::UnitY, angle);

    transform = Matrix::CreateScale(scale) * Matrix::CreateFromQuaternion(rotation) * Matrix::CreateTranslation(translation);

    character->setRootTransform(transform);
}

