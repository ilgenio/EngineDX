#include "Globals.h"

#include "ModuleCamera.h"

#include "Application.h"
#include "ModuleD3D12.h"

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
    ModuleD3D12* d3d12 = app->getD3D12();

    unsigned width = d3d12->getWindowWidth();
    unsigned height = d3d12->getWindowHeight();

    position = Vector3(0.0f, 0.0f, 10.0f);
    rotation = Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), XMConvertToRadians(0.0f));

    Quaternion invRot;
    rotation.Inverse(invRot);

    view = Matrix::CreateFromQuaternion(invRot);
    view.Translation(-position);

    view  = Matrix::CreateLookAt(Vector3(0.0f, 0.0f, 10.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
    proj  = Matrix::CreatePerspectiveFieldOfView(XM_PIDIV4, float(width) / float(height), NEAR_PLANE, FAR_PLANE);

    return true;
}

UpdateStatus ModuleCamera::update()
{
    Mouse& mouse = Mouse::Get();
    Keyboard& keyboard = Keyboard::Get();
    GamePad& pad = GamePad::Get();
    
    const Mouse::State& mouseState = mouse.GetState();
    const Keyboard::State& keyState = keyboard.GetState();

    int padId = 0;
    for (padId; padId < GamePad::MAX_PLAYER_COUNT ; ++padId)
    {
        if (pad.GetCapabilities(padId).connected) break;
    }
                
    const GamePad::State& padState = pad.GetState(padId);

    if(mouseState.leftButton)
    {
        if (leftDrag)
        {
            int relX = mouseState.x - dragPosX;
            int relY = mouseState.y - dragPosY;

            if (keyState.LeftControl)
            {
                dragging.panning.x = -getTranslationSpeed() * params.radius * relX;
                dragging.panning.y = getTranslationSpeed() * params.radius * relY;
            }
            else
            {
                dragging.polar = XMConvertToRadians(getRotationSpeed() * relX);
                dragging.azimuthal =  XMConvertToRadians(getRotationSpeed() * relY);
            }
        }
        else
        {
            leftDrag = true;
            dragPosX = mouseState.x;
            dragPosY = mouseState.y;
        }
    }
    else
    {
        leftDrag = false;

        Quaternion rotation_polar = Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), dragging.polar + params.polar);
        Quaternion rotation_azimuthal = Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), dragging.azimuthal + params.azimuthal);
        Quaternion rotation = rotation_polar*rotation_azimuthal;

        params.polar        += dragging.polar;
        params.azimuthal    += dragging.azimuthal;
        
        params.panning      += Vector3::Transform(dragging.panning, rotation);
        dragging.polar      = 0.0f;
        dragging.azimuthal  = 0.0f;
        dragging.panning    = Vector3(0.0f);
    }

    if(!leftDrag && mouseState.rightButton)
    {
        if (rightDrag)
        {            
            int relY = mouseState.y - dragPosY;

            dragging.radius = getTranslationSpeed() * params.radius * relY;
            if (dragging.radius < -params.radius)
            {
                dragging.radius = -params.radius;
            }
        }
        else
        {
            rightDrag = true;
            dragPosY = mouseState.y;
        }
    }
    else
    {
        rightDrag = false;
        params.radius += dragging.radius;
        dragging.radius = 0.0f;
    }

    // Updates camera view 

    Quaternion rotation_polar     = Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), dragging.polar + params.polar);
    Quaternion rotation_azimuthal = Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), dragging.azimuthal + params.azimuthal);
    rotation = rotation_polar*rotation_azimuthal;
    position = Vector3::Transform((Vector3(0.0f, 0.0f, dragging.radius + params.radius) + dragging.panning) + params.panning, rotation );

    Quaternion invRot;
    rotation.Inverse(invRot);

    view = Matrix::CreateFromQuaternion(invRot); 
    view.Translation(-position); 
    
    return UPDATE_CONTINUE;
}

void ModuleCamera::windowResized(unsigned newWidth, unsigned newHeight)
{
    proj = Matrix::CreatePerspectiveFieldOfView(XM_PIDIV4, float(newWidth) / float(newHeight), NEAR_PLANE, FAR_PLANE);
}

