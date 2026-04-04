#pragma once

#include "Module.h"

class DemoScene : public Module
{
public:

    bool cleanUp() override;
    bool init() override;
};
