#pragma once

#include <vector>
#include <span>
#include <filesystem>

class Model;
class Mesh;
class Material;
class QuadTree;

namespace tinygltf { class Model;  class Node; }

struct RenderMesh
{
    const Mesh *mesh = nullptr;
    const Material *material = nullptr;
    const Matrix* palette = nullptr;
    UINT numJoints = 0;
    UINT skinningOffset = 0;

    Matrix transform;
    Matrix normalMatrix;
};

class Scene
{
private:
    friend class Model;
    friend class Material;

    struct SharedTexture
    {
        UINT count = 0;
        ComPtr<ID3D12Resource> texture;
    };

    std::vector<Model*> models;
    std::unordered_map<std::filesystem::path, SharedTexture> textures;

    std::unique_ptr<QuadTree> quadTree;

public:
    Scene();
    ~Scene();

    Model* loadModel(const char* fileName, const char* basePath);

    void updateAnimations(float deltaTime);
    void updateWorldTransforms();
    void frustumCulling(Vector4 planes[6], std::vector<RenderMesh>& renderList);

    void debugDrawQuadTree(const Vector4 planes[6], UINT level) const;

private:
    void onRemoveModel(Model* model);
    
    ComPtr<ID3D12Resource> loadTexture(const std::filesystem::path& path, bool defaultSRGB = false);
    void unloadTexture(const std::filesystem::path& path);

};
