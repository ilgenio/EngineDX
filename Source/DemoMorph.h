#pragma once

#include "Module.h"

class DemoMorph : public Module
{
    enum Clips
    {
        CLIP_INDIVIDUAL = 0,
        CLIP_WAVE,
        CLIP_PULSE,
        CLIP_COUNT
    };

    UINT modelIdx = 0;
    UINT clips[CLIP_COUNT];
    UINT active = 0;

public:

    bool init() override;
    void preRender() override;

};
