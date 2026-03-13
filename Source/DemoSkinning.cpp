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
#include "GamePad.h"

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
    character->playAnim(scene->getClip(anims[IDLE_LONG]));
    currentAnim = IDLE_LONG;

    ModuleRender* render = app->getRender();
    render->addDebugDrawModel(modelIdx);

    ModuleCamera* camera = app->getCamera();

    camera->setPolar(XMConvertToRadians(-1.20f));
    camera->setAzimuthal(XMConvertToRadians(-30.0f));
    camera->setTranslation(Vector3(0.0f, 2.3f, 1.0f));    
    camera->setEnableInput(false); 

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
            character->playAnim(scene->getClip(anims[TPOSE]));
            currentAnim = TPOSE;
        }
        else
        {
            character->playAnim(scene->getClip(anims[IDLE_LONG]));
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
    ImGui::InputFloat("Angular Speed", &angularSpeed);

    if (ImGui::Checkbox("Camera track", &cameraTrack))
    {
        ModuleCamera* camera = app->getCamera();

        if(cameraTrack)
        {
            ModuleScene* scene = app->getScene();

            std::shared_ptr<Model> character = scene->getModel(modelIdx);

            const Vector3& cameraPos = camera->getCamera().Translation();
            const Vector3& characterPos = character->getRootTransform().Translation();

            trackOffset = cameraPos - characterPos;
            camera->setEnableInput(false);
        }
        else
        {
            camera->setEnableInput(true);
        }
    }


    ImGui::End();
}

void DemoSkinning::update()
{
    GamePad& pad = GamePad::Get();
    GamePad::State padState = pad.GetState(0);

    Vector2 localPadDir = Vector2::Zero;

    if (padState.IsConnected())
    {
        localPadDir = Vector2(padState.thumbSticks.leftX, -padState.thumbSticks.leftY);
    }

    Keyboard& keyboard = Keyboard::Get();
    const Keyboard::State& keyState = keyboard.GetState();

    bool inputMovementFromPad = localPadDir.LengthSquared() > 1e-6;
    bool inputWantsMovement =  inputMovementFromPad || keyState.Up;
    bool inputWantsShot = padState.IsRightTriggerPressed() || keyState.Enter;

    auto doMovement = [localPadDir, inputMovementFromPad, keyState, this]()
        {
            moveCharacter();
            if (inputMovementFromPad)
            {
                rotateCharacterFromPadDir(localPadDir);
            }
            else
            {
                if (keyState.Left)
                {
                    rotateCharacterFromLocalDir(localLeft);
                }
                if (keyState.Right)
                {
                    rotateCharacterFromLocalDir(localRight);
                }
            }
        };

    switch (currentAnim)
    {
        case IDLE:
        case IDLE_LONG:
        {
            if (inputWantsShot)
            {
                setAnimation(SHOT);
            }
            else if (inputWantsMovement)
            {
                setAnimation(RUN);
                doMovement();
            }
            break;
        }
        case RUN:
        {
            if (inputWantsShot)
            {
                setAnimation(SHOT);
            }
            else if (inputWantsMovement)
            {
                doMovement();
            }
            else
            {
                setAnimation(IDLE_LONG);
            }
            break;
        }
        case SHOT:
        {
            ModuleScene* scene = app->getScene();
            std::shared_ptr<Model> character = scene->getModel(modelIdx);
            float animTime = character->getAnimTime();
            float animDuration = character->getAnimDuration();

            if (animDuration < 1e-5 || (animTime / animDuration) >= 0.9f)
            {
                setAnimation(IDLE);
            }
            break;
        }
    }

    if (cameraTrack)
    {
        ModuleCamera* camera = app->getCamera();

        ModuleScene* scene = app->getScene();
        std::shared_ptr<Model> character = scene->getModel(modelIdx);

        const Matrix& transform = character->getRootTransform();
        camera->setTranslation(transform.Translation() + trackOffset);
    }
}

void DemoSkinning::setAnimation(Anims anim)
{
    ModuleScene* scene = app->getScene();
    std::shared_ptr<Model> character = scene->getModel(modelIdx);
    character->playAnim(scene->getClip(anims[anim]), anim != SHOT, 0.2f);
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

void DemoSkinning::rotateCharacterFromPadDir(const Vector2& padDir)
{
    ModuleCamera* camera = app->getCamera();
    ModuleScene* scene = app->getScene();
    std::shared_ptr<Model> character = scene->getModel(modelIdx);

    Vector3 localCameraDir = Vector3(padDir.x, 0.0, padDir.y);

    const Matrix& cameraTransform = camera->getCamera();
    const Matrix& characterTransform = character->getRootTransform();

    Vector3 targetDir        = Vector3::TransformNormal(localCameraDir, cameraTransform);
    Vector3 characterForward = Vector3::TransformNormal(Vector3::UnitZ, characterTransform);

    rotateCharacter(targetDir, characterForward);
}

void DemoSkinning::rotateCharacterFromLocalDir(const Vector3& localDir)
{
    rotateCharacter(localDir, localForward);
}

void DemoSkinning::rotateCharacter(const Vector3& target, const Vector3& current)
{
    ModuleScene* scene = app->getScene();
    std::shared_ptr<Model> character = scene->getModel(modelIdx);

    float elapsedSec = app->getElapsedMilis() * 0.001f;

    float angleDiff = atan2f(target.x, target.z) - atan2f(current.x, current.z);
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

