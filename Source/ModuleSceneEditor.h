#pragma once

#include "Module.h"

struct Directional;
struct Point;
struct Spot;

class ModuleSceneEditor : public Module
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
    ModuleSceneEditor();
    ~ModuleSceneEditor();

    void render() override;

private:    

    void imGuiDrawObjects();
    void imGuiDrawProperties();
    void imGuiDrawLightProperties();
    bool imGuiDrawDirectionalProperties(Directional& dirLight);
    bool imGuiDrawPointProperties(Point& pointLight);
    bool imGuiDrawSpotProperties(Spot& spotLight);
    void renderDebugDrawModel();
    void renderDebugDrawLight();
};