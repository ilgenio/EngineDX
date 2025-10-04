#include "Globals.h"

#include "ModuleCamera.h"

#include "Application.h"

#include "Mouse.h"
#include "Keyboard.h"
#include "GamePad.h"

#define FAR_PLANE 20000.0f
#define NEAR_PLANE 0.1f

namespace
{
    constexpr float getRotationSpeed() { return 25.0f; }
    constexpr float getTranslationSpeed() { return 2.5f; } 
}

bool ModuleCamera::init()
{
    position = Vector3(0.0f, 0.0f, 10.0f);
    rotation = Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), XMConvertToRadians(0.0f));

    Quaternion invRot;
    rotation.Inverse(invRot);

    view = Matrix::CreateFromQuaternion(invRot);
    view.Translation(-position);

    view = Matrix::CreateLookAt(Vector3(0.0f, 0.0f, 10.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));

    return true;
}

void ModuleCamera::update()
{
    Mouse& mouse = Mouse::Get();
    const Mouse::State& mouseState = mouse.GetState();

    if (enabled)
    {
        Keyboard& keyboard = Keyboard::Get();
        GamePad& pad = GamePad::Get();

        const Keyboard::State& keyState = keyboard.GetState();
        GamePad::State padState = pad.GetState(0);
       
        float elapsedSec = app->getElapsedMilis()*0.001f;

        Vector3 translate = Vector3::Zero;
        Vector2 rotate = Vector2::Zero;

        // Check game pad
        if (padState.IsConnected())
        {
            rotate.x = -padState.thumbSticks.rightX * elapsedSec;
            rotate.y = -padState.thumbSticks.rightY * elapsedSec;

            translate.x = padState.thumbSticks.leftX  * elapsedSec;
            translate.z = -padState.thumbSticks.leftY * elapsedSec;

            if (padState.IsLeftTriggerPressed())
            {
                translate.y = 0.25f * elapsedSec;
            }
            else if (padState.IsRightTriggerPressed())
            {
                translate.y -= 0.25f * elapsedSec;
            }
        }
        
        if (mouseState.leftButton)
        {
            rotate.x = float(dragPosX - mouseState.x) * 0.005f;
            rotate.y = float(dragPosY - mouseState.y) * 0.005f;
        }

        if (keyState.W) translate.z -= 0.45f * elapsedSec;
        if (keyState.S) translate.z += 0.45f * elapsedSec;
        if (keyState.A) translate.x -= 0.45f * elapsedSec;
        if (keyState.D) translate.x += 0.45f * elapsedSec;
        if (keyState.Q) translate.y += 0.45f * elapsedSec;
        if (keyState.E) translate.y -= 0.45f * elapsedSec;


        Vector3 localDir = Vector3::Transform(translate, rotation);
        params.panning += localDir * getTranslationSpeed();
        params.polar += XMConvertToRadians(getRotationSpeed() * rotate.x);
        params.azimuthal += XMConvertToRadians(getRotationSpeed() * rotate.y);

            
        // Updates camera view 

        Quaternion rotation_polar = Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), params.polar);
        Quaternion rotation_azimuthal = Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), params.azimuthal);
        rotation = rotation_azimuthal * rotation_polar;
        position = params.panning; // Vector3::Transform(params.panning, rotation);

        Quaternion invRot;
        rotation.Inverse(invRot);

        view = Matrix::CreateFromQuaternion(invRot);
        view.Translation(Vector3::Transform(-position, invRot));

    }

    dragPosX = mouseState.x;
    dragPosY = mouseState.y;
}

Matrix ModuleCamera::getPerspectiveProj(float aspect) 
{
    return Matrix::CreatePerspectiveFieldOfView(XM_PIDIV4, aspect, NEAR_PLANE, FAR_PLANE);
}

