#pragma once

#include <vector>

class Scene;

struct RenderItem
{
    uint32_t instance = UINT32_MAX;
    float    cameraDist = 0.0f;
};

typedef std::vector<RenderItem> RenderItemList;

class RenderList
{
    RenderItemList opaqueList;
    RenderItemList blendList;

public:

    RenderList();
    ~RenderList();

    void update(const Scene* scene, const Vector3& cameraPos);

    const RenderItemList& getOpaqueList() const {return opaqueList;}
    const RenderItemList& getBlendList() const {return blendList;}


};