#pragma once

#include <vector>
#include <span>

class Mesh;
class Material;
class Skybox;

namespace tinygltf { class Model;  class Node; }

struct RenderMesh
{
    const Mesh *mesh = nullptr;
    const Material *material = nullptr;
    Matrix worldTransform;
};

class Scene
{
private:
    struct Node
    {
        std::string name;
        Matrix localTransform;
        Matrix worldTransform;
        bool dirtyWorld = true;
        INT parent = -1;
    };

    struct MeshInstance
    {
        UINT meshIndex = 0;
        UINT materialIndex = 0;
        INT  skinIndex = 0;
        UINT nodeIndex = 0;
    };

    typedef std::vector<Mesh*> MeshList;
    typedef std::vector<Material*> MaterialList;
    typedef std::vector<Node> NodeList;
    typedef std::vector<MeshInstance> InstanceList;

    MeshList     meshes;
    MaterialList materials;
    NodeList     nodes;
    InstanceList instances;

public:
    Scene();
    ~Scene();

    void loadSkybox(const char* background, const char* diffuse, const char* specular, const char* brdf);
    bool load(const char* fileName, const char* basePath);

    // TODO: Use std::span
    std::span<const Mesh* const>     getMeshes() const { return std::span<const Mesh* const>(meshes.data(), meshes.size()); }
    std::span<const Material* const> getMaterials() const { return std::span<const Material* const>(materials.data(), materials.size()); }

    void updateWorldTransforms();
    void getRenderList(std::vector<RenderMesh>& renderList) const;

private:

    bool load(const tinygltf::Model& srcModel, const char* basePath);
    void generateNodes(const tinygltf::Model& model, UINT nodeIndex, INT parentIndex, const std::vector<int>& materialMapping,
                        UINT nodeOffset, UINT meshOffset, UINT materialOffset);
    
private:
};
