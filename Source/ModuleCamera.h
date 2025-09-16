#pragma once

#include "Module.h"

//-----------------------------------------------------------------------------
// ModuleCamera manages the application's main camera.
// It handles camera position, rotation, and view matrix updates based on
// mouse, keyboard, and gamepad input. Provides methods to access the
// current view matrix, orientation, and position, as well as to configure
// perspective projection.
//-----------------------------------------------------------------------------
class ModuleCamera : public Module
{
    struct Params
    {
        float  polar;
        float  azimuthal;
        Vector3 panning;
    };

    Params    params = { 0.0f, 0.0f , {0.0f , 2.0f , 10.0f }};
    int       dragPosX = 0;
    int       dragPosY = 0;

    Quaternion rotation;
    Vector3 position;
    Matrix view;
    bool enabled = true;
public:

    bool init() override;
    void update() override;

    void setEnable(bool flag) { enabled = flag; }
    bool getEnabled() const { return enabled;  }

    const Matrix&     getView() const {return view;}
    const Quaternion& getRot() const { return rotation; }
    const Vector3&    getPos() const { return position; }

    static Matrix getPerspectiveProj(float aspect); 

};