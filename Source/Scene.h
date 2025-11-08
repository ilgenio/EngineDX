#pragma once

#include <vector>
#include <span>

class Model;
class Mesh;
class Material;
class QuadTree;

namespace tinygltf { class Model;  class Node; }

struct RenderMesh
{
    const Mesh *mesh = nullptr;
    const Material *material = nullptr;
    Matrix transform;
    Matrix normalMatrix;
};

class Scene
{
private:
    friend class Model;

    std::vector<Model*> models;
    std::unique_ptr<QuadTree> quadTree;

public:
    Scene();
    ~Scene();

    Model* loadModel(const char* fileName, const char* basePath);

    void updateAnimations(float deltaTime);
    void updateWorldTransforms();
    void frustumCulling(Vector4 planes[6], std::vector<RenderMesh>& renderList);

    void debugDrawQuadTree(const Vector4 frustumPlanes[6]) const;

private:
    void onRemoveModel(Model* model);

    
};
