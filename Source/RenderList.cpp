#include "Globals.h"

#include "RenderList.h"
#include "Scene.h"
#include "Mesh.h"
#include "Material.h"

RenderList::RenderList()
{

}

RenderList::~RenderList()
{

}

void RenderList::update(const Scene *scene, const Vector3& cameraPos)
{
    opaqueList.clear();
    blendList.clear();
    opaqueList.reserve(scene->getNumInstances());
    blendList.reserve(scene->getNumInstances());

    struct MinDist { bool operator()(const RenderItem& left, float dist) const { return left.cameraDist <= dist; } };
    struct MaxDist { bool operator()(const RenderItem& left, float dist) const { return left.cameraDist >= dist; } };

    for(uint32_t i=0, count = scene->getNumInstances(); i < count; ++i)
    {
        const MeshInstance& instance = scene->getInstance(i);
        const Mesh* mesh = scene->getMesh(instance.meshIndex);
        const Material* material = scene->getMaterial(mesh->getMaterialIndex());

        Vector3 pos = instance.transformation.Translation();
        float dist = Vector3::Distance(cameraPos, pos);

        if(material->getAlphaMode() == Material::ALPHA_MODE_BLEND)
        {
            auto it = std::lower_bound(blendList.begin(), blendList.end(), dist, MaxDist());

            blendList.insert(it, RenderItem({ i, dist }));
        }
        else
        {
            auto it = std::lower_bound(opaqueList.begin(), opaqueList.end(), dist, MinDist());

            opaqueList.insert(it, RenderItem({ i, dist }));
        }
    }
}

