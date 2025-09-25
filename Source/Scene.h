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

//-----------------------------------------------------------------------------
// Scene manages the collection of meshes, materials, and nodes for a 3D scene.
// It supports loading scene data from glTF files, organizing node hierarchies,
// updating world transforms, and preparing render lists for rendering.
//-----------------------------------------------------------------------------
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
        UINT numChilds = 0;
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
    typedef std::vector<Node*> NodeList;
    typedef std::vector<MeshInstance*> InstanceList;

    MeshList     meshes;
    MaterialList materials;
    NodeList     nodes;
    InstanceList instances;

public:
    Scene();
    ~Scene();

    void loadSkybox(const char* background, const char* diffuse, const char* specular, const char* brdf);
    bool load(const char* fileName, const char* basePath);

    void updateWorldTransforms();
    void getRenderList(std::vector<RenderMesh>& renderList) const;

private:

    bool load(const tinygltf::Model& srcModel, const char* basePath);
    UINT generateNodes(const tinygltf::Model& model, UINT nodeIndex, INT parentIndex, 
                       const std::vector<std::pair<UINT, UINT> >& meshMapping, 
                       const std::vector<int>& materialMapping);
    
private:
};
