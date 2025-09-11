#pragma once

#include <vector>

class Mesh;
class Material;
class Skybox;

namespace tinygltf { class Model;  class Node; }

struct MeshInstance
{
    uint32_t meshIndex = 0;
    Matrix transformation;
};

class Scene
{
public:

public:
    Scene();
    ~Scene();

    void loadSkybox(const char* background, const char* diffuse, const char* specular, const char* brdf);
    void load(const tinygltf::Model& srcModel, const char* basePath);

    uint32_t getNumMeshes() const {return uint32_t(meshes.size());}
    const Mesh* getMesh(uint32_t index) const {return meshes[index].get(); }

    uint32_t getNumMaterials() const {return uint32_t(materials.size());}
    const Material* getMaterial(uint32_t index ) const {return materials[index].get();}

    uint32_t getNumInstances() const {return uint32_t(instances.size());}
    const MeshInstance& getInstance(uint32_t index) const {return instances[index];}

  //  const Skybox* getSkybox() const {return skybox.get();}

private:

    void generateInstancesRec(const tinygltf::Model& srcModel, int nodeIndex, const Matrix& transform);
    
private:
    std::vector<std::unique_ptr<Mesh> >     meshes;
    std::vector<std::unique_ptr<Material> > materials;
    std::vector<MeshInstance>               instances;
    //std::unique_ptr<Skybox>                 skybox;
};