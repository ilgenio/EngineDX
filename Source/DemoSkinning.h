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
public:

    virtual bool init() override;
    virtual void preRender() override;
    virtual void update() override;
};

