#pragma once

#include "Module.h"

class ModuleCamera : public Module
{
	struct Params
	{
		float  radius;
        float  polar;
        float  azimuthal;
        Vector3 panning;
	};

	Params    params = { 10.0f ,0.0f, 0.0f , {0.0f , 0.0f , 0.0f }};
	Params    dragging = { 0.0f ,0.0f, 0.0f , {0.0f , 0.0f , 0.0f } };;
    bool      leftDrag = false;
    bool      rightDrag = false;
    int       dragPosX = 0;
    int       dragPosY = 0;

    Quaternion rotation;
    Vector3 position;
    Matrix view;
    Matrix proj;
    bool enabled = true;
public:

	bool init() override;
	void update() override;

    void windowResized(unsigned newWidth, unsigned newHeight);

    void setEnable(bool flag) { enabled = flag; }
    bool getEnabled() const { return enabled;  }

    const Matrix&     getView() const {return view;}
    const Matrix&     getProj() const {return proj;}
    const Vector3&    getPos() const  {return position;}
    const Quaternion& getRot() const  {return rotation;}

};