#include "Globals.h"

#include "ModuleCamera.h"

#include "Application.h"
#include "ModuleD3D12.h"

#include "Mouse.h"
#include "Keyboard.h"

#define FAR_PLANE 20000.0f
#define NEAR_PLANE 0.1f

namespace
{
    constexpr float getRotationSpeed() { return 0.075f; }
    constexpr float getTranslationSpeed() { return 0.003f; }
}

bool ModuleCamera::init()
{
    unsigned width, height;
    app->getD3D12()->getWindowSize(width, height);

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
    
    const Mouse::State& mouseState = mouse.GetState();
    const Keyboard::State& keyState = keyboard.GetState();

    if(mouseState.leftButton)
    {
        if (mouseState.positionMode == Mouse::MODE_RELATIVE)
        {
            float sign_x = mouseState.x < 0.0f ? -1.0f : 1.0f;
            float sign_y = mouseState.y < 0.0f ? -1.0f : 1.0f;

            if (keyState.LeftControl)
            {
                dragging.panning.x += -getTranslationSpeed() * params.radius * mouseState.x;
                dragging.panning.y += getTranslationSpeed() * params.radius * mouseState.y;
            }
            else
            {
                dragging.polar += -sign_x * XMConvertToRadians(getRotationSpeed() * mouseState.x);
                dragging.azimuthal += sign_y * XMConvertToRadians(getRotationSpeed() * mouseState.x);
            }
        }
        else
        {
            mouse.SetMode(Mouse::MODE_RELATIVE);
        }
    }
    else
    {
        mouse.SetMode(Mouse::MODE_ABSOLUTE);

        Quaternion rotation_polar = Quaternion::CreateFromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), dragging.polar + params.polar);
        Quaternion rotation_azimuthal = Quaternion::CreateFromAxisAngle(Vector3(1.0f, 0.0f, 0.0f), dragging.azimuthal + params.azimuthal);
        Quaternion rotation = rotation_polar*rotation_azimuthal;

        params.polar        += dragging.polar;
        params.azimuthal    += dragging.azimuthal;
        params.panning      += rotation * dragging.panning;
        dragging.polar      = 0.0f;
        dragging.azimuthal  = 0.0f;
        dragging.panning    = Vector3(0.0f);
    }

    if(mouseState.rightButton)
    {
        if (mouseState.positionMode == Mouse::MODE_RELATIVE)
        {
            dragging.radius += getTranslationSpeed() * params.radius * mouseState.y;
            if (dragging.radius < -params.radius)
            {
                dragging.radius = -params.radius;
            }
        }
        else
        {
            mouse.SetMode(Mouse::MODE_RELATIVE);
        }
    }
    else
    {
        mouse.SetMode(Mouse::MODE_ABSOLUTE);
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

