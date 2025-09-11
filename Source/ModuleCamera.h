#pragma once

#include "Module.h"

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