#pragma once

#include "Module.h"

class DemoScene : public Module
{
    enum SelectionType
    {
        SELECTION_NONE = 0,
        SELECTION_MODEL,
        SELECTION_LIGHT,
    };

    SelectionType selectionType = SELECTION_NONE;
    UINT selectedIndex = UINT_MAX;

public:

    bool cleanUp() override;
    bool init() override;
    void preRender() override;

private:
    void serialize();
    bool deserialize();
    void addLights();
};
