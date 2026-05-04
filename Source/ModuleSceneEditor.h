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
        SELECTION_DECAL,
    };

    bool showAxis = false;
    bool showGrid = false;
    bool showQuadTree = false;
    bool trackFrustum = false;

    Vector4 frustumPlanes[6];
    Vector3 trackedFrustumCorners[8];
    UINT quadTreeLevel = 0;

    SelectionType selectionType = SELECTION_NONE;
    UINT selectedIndex = UINT_MAX;
    bool showSpotSphere = false;

public:
    ModuleSceneEditor();
    ~ModuleSceneEditor();

    Json::object serialize() const;
    void deserialize(const Json& obj);

    void render() override;

    bool getShowAxis() const { return showAxis; }
    bool getShowGrid() const { return showGrid; }
    bool getShowQuadtree() const { return showQuadTree; }
    bool getTrackFrustum() const { return trackFrustum; }

    void setShowAxis(bool value) { showAxis = value; }
    void setShowGrid(bool value) { showGrid = value; }
    void setShowQuadtree(bool value) { showQuadTree = value; }
    void setTrackFrustum(bool value) { trackFrustum = value; }

private:    

    void debugDrawCommands();
    void imGuiDrawObjects();
    void imGuiDrawProperties();
    void imGuiDrawLightProperties();
    void imGuiDrawDecalProperties();
    bool imGuiDrawDirectionalProperties(Directional& dirLight);
    bool imGuiDrawPointProperties(Point& pointLight);
    bool imGuiDrawSpotProperties(Spot& spotLight);
    void renderDebugDrawModel();
    void renderDebugDrawLight();
    void renderDebugDrawDecal();
};