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
    constexpr float getRotationSpeed() { return 0.075f; }
    constexpr float getTranslationSpeed() { return 0.003f; }
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
       
        enum EAction { ROTATING, PANNING, ZOOMING, NONE } action = NONE;
        float relX = 0.0f;
        float relY = 0.0f;

        // Check game pad
        if (padState.IsConnected())
        {
            relX = -padState.thumbSticks.rightX;
            relY = -padState.thumbSticks.rightY;

            if (relX != 0.0f || relY != 0.0f)
            {
                action = ROTATING;
            }
            else
            {
                relX = -padState.thumbSticks.leftX*0.25f;

                if (padState.IsLeftTriggerPressed())
                {
                    relY += 0.1f;
                }
                else if (padState.IsRightTriggerPressed())
                {
                    relY -= 0.1f;
                }

                if (relX != 0.0f || relY != 0.0f)
                {
                    action = PANNING;
                }
                else
                {
                    relY = -padState.thumbSticks.leftY * 0.25f;
                    if (relY != 0.0f)
                    {
                        action = ZOOMING;
                    }
                }
            }
        }
        
        // Check mouse and keyboard
        if (action == NONE )
        {
            if (mouseState.leftButton)
            {
                relX = float(dragPosX - mouseState.x);
                relY = float(dragPosY - mouseState.y);
                action = (keyState.LeftControl) ? PANNING : ROTATING;
            }
   
            if (mouseState.rightButton)
            {
                relY = float(mouseState.y - dragPosY);
                action = ZOOMING;
            }
        }

        switch (action)
        {
            case PANNING:
                params.panning.x += -getTranslationSpeed() * params.radius * relX;
                params.panning.y += getTranslationSpeed() * params.radius * relY;
                break;
            case ROTATING:
                params.polar += XMConvertToRadians(getRotationSpeed() * relX);
                params.azimuthal += XMConvertToRadians(getRotationSpeed() * relY);
                break;
            case ZOOMING:
                params.radius += getTranslationSpeed() * params.radius * relY;

                break;
            default:
                break;
        }

        // Updates camera view 

        Quaternion rotation_polar = Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), params.polar);
        Quaternion rotation_azimuthal = Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), params.azimuthal);
        rotation = rotation_azimuthal * rotation_polar;
        position = Vector3::Transform(Vector3(0.0f, 0.0f, params.radius) + params.panning, rotation);

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

