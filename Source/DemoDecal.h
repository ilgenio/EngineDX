#pragma once

#include "Module.h"
#include "ImGuizmo.h"

class DemoDecal : public Module
{

public:

    bool cleanUp() override;
    bool init() override;

private:
    void serialize();
    bool deserialize();
};
