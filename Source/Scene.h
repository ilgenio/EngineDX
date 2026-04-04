#pragma once

#include <vector>
#include <span>
#include <filesystem>

class Model;
class Mesh;
class Material;
class QuadTree;
class Light;
struct Directional;
struct Point;
struct Spot;

namespace tinygltf { class Model;  class Node; }

struct RenderMesh
{
    const Mesh *mesh = nullptr;
    const Material *material = nullptr;
    const Matrix* palette = nullptr;
    UINT numJoints = 0;
    UINT skinningOffset = 0;
    const float* morphWeights = nullptr;
    UINT numMorphTargets = 0;

    Matrix transform;
    Matrix normalMatrix;
};

class Scene
{
private:
    friend class Model;
    friend class Light;
    friend class Material;

    struct SharedTexture
    {
        UINT count = 0;
        ComPtr<ID3D12Resource> texture;
    };

    std::vector<Model*> models;
    std::vector<Light*> lights;

    std::unordered_map<std::filesystem::path, SharedTexture> textures;
    std::unique_ptr<QuadTree> quadTree;

public:
    Scene();
    ~Scene();

    Model* loadModel(const char* fileName, const char* basePath);

    void   addLight(const Directional& directional);
    void   addLight(const Point& point);
    void   addLight(const Spot& spot);

    void updateAnimations(float deltaTime);
    void updateWorldTransforms();
    void frustumCulling(Vector4 planes[6], std::vector<RenderMesh>& renderList);

    void debugDrawQuadTree(const Vector4 planes[6], UINT level) const;

private:
    void onRemoveModel(Model* model);
    void onRemoveLight(Light* light);
    
    ComPtr<ID3D12Resource> loadTexture(const std::filesystem::path& path, bool defaultSRGB = false);
    void unloadTexture(const std::filesystem::path& path);

};
