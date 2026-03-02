#pragma once

#include "Module.h"

class DemoSkinning : public Module
{
    enum Anims
    {
        TPOSE = 0,
        IDLE,
        IDLE_LONG,
        RUN,
        SHOT,
        ANIM_COUNT
    };

    UINT modelIdx = UINT(-1);
    UINT anims[ANIM_COUNT] = { UINT(-1) };
    UINT currentAnim = ANIM_COUNT;
    bool showTPose = false;
    float linearSpeed = 4.0f; 
    float angularSpeed = M_PI;
    const Vector3 localForward = Vector3::UnitZ;
    const Vector3 localLeft = Vector3::UnitX;
    const Vector3 localRight = -Vector3::UnitX;
public:

    virtual bool init() override;
    virtual void preRender() override;
    virtual void update() override;

private:

    void setAnimation(Anims anim);
    void moveCharacter();
    void rotateCharacter(const Vector3& localDir);
};

