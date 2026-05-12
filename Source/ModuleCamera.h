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
        Vector3 translation;
    };

    Params    params = { 0.0f, 0.0f , {0.0f , 2.0f , 10.0f }};
    Params    tmpParams = { 0.0f, 0.0f , {0.0f , 0.0f , 0.0f } };
    int       dragPosX = 0;
    int       dragPosY = 0;

    Quaternion rotation;
    Vector3 position;
    Matrix view;
    Matrix camera;
    bool enabled = true;
    bool enableInput = true; 
    UINT64  lastMilis = 0;
    UINT64  elapsedMilis = 0;
    float nearPlane = 0.1f;
    float farPlane = 250.0f;
    float fov = XM_PIDIV4;

public:

    bool init() override;
    void update() override;

    void serialize(Json& obj) const;
    void deserialize(const Json& obj);

    void setEnable(bool flag) { enabled = flag; }
    bool getEnabled() const { return enabled;  }

    void setEnableInput(bool flag) { enableInput = flag; }
    bool getEnableInput() const { return enableInput;  }

    float getPolar() const { return params.polar; }
    float getAzimuthal() const { return params.azimuthal;  }
    const Vector3& getTranslation() const { return params.translation; }

    void setPolar(float polar) { params.polar = polar;  }
    void setAzimuthal(float azimuthal) { params.azimuthal = azimuthal;  }
    void setTranslation(const Vector3& translation) { params.translation = translation;  }

    const Matrix&     getCamera() const { return camera; }
    const Matrix&     getView() const {return view;}
    const Quaternion& getRot() const { return rotation; }
    const Vector3&    getPos() const { return position; }


    float getFov() const { return fov; }
    float getNearPlane() const { return nearPlane; }
    float getFarPlane() const { return farPlane; }


    Matrix getPerspectiveProj(float aspect) const;
    void getFrustumPlanes(Vector4 planes[6], float aspect, bool normalize) const;
    void getFrustumCorners(Vector3 corners[8], float aspect) const;

    static void getFrustumCorners(Vector3 corners[8], const Matrix& proj, const Matrix& invView, bool perspectiveProj);
};
