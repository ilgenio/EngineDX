#pragma once

#include <vector>
#include <span>

class Model;
class Mesh;
class Material;

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

public:
    Scene();
    ~Scene();

    Model* loadModel(const char* fileName, const char* basePath);

    void updateWorldTransforms();
    void getRenderList(std::vector<RenderMesh>& renderList) const;

private:
    void onRemoveModel(Model* model);

    
};
