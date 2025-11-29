#pragma once 

#include <vector>
#include <string>

class Scene;
class Mesh;
class Material;
struct RenderMesh;
class QuadTree;

namespace tinygltf { class Model;  class Node; }

class Model
{
    friend class Scene;

    struct Node
    {
        std::string name;
        Matrix localTransform;
        Matrix worldTransform;
        bool dirtyWorld = true;
        bool lastFrameDirty = true;
        INT parent = -1;
        UINT numChilds = 0;
    };

    struct MeshInstance
    {
        UINT meshIndex = 0;
        UINT materialIndex = 0;
        INT  skinIndex = 0;
        UINT nodeIndex = 0;
        UINT quadTreeCell = UINT(-1);
    };

    struct AnimInstance
    {
        std::shared_ptr<class AnimationClip> clip;
        float time = 0.0f;
        float fadeIn = 0.0f;

        std::unique_ptr<AnimInstance> next;
    };

    typedef std::vector<Mesh*> MeshList;
    typedef std::vector<Material*> MaterialList;
    typedef std::vector<Node*> NodeList;
    typedef std::vector<MeshInstance*> InstanceList;

    MeshList meshes;
    MaterialList materials;
    NodeList nodes;
    InstanceList instances;
    std::unique_ptr<AnimInstance> currentAnim;
    Scene* scene = nullptr;
    std::string name;

public:
    ~Model();

    void PlayAnim(std::shared_ptr<AnimationClip> clip, float fadeIn = 0.0f);
    void StopAnim();

    const std::string& getName() const { return name; }

    UINT getNumMeshes() const { return UINT(meshes.size()); }
    UINT getNumMaterials() const { return UINT(materials.size()); }
    UINT getNumNodes() const { return UINT(nodes.size()); }
    UINT getNumInstances() const { return UINT(instances.size()); }

private:

    explicit Model(Scene* parentScene, const char* name);

    bool load(const tinygltf::Model& srcModel, const char* basePath);
    void updateAnim(float deltaTime);
    void updateWorldTransforms();
    void updateQuadTree(QuadTree* quadTree, bool force);
    void frustumCulling(const Vector4 frustumPlanes[6], const Vector3 absFrustumPlanes[6], 
                        const std::vector<IntersectionType>& containment, 
                        std::vector<RenderMesh>& renderList) const;

    UINT generateNodes(const tinygltf::Model& model, UINT nodeIndex, INT parentIndex, 
                       const std::vector<std::pair<UINT, UINT> >& meshMapping, 
                       const std::vector<int>& materialMapping);

};
