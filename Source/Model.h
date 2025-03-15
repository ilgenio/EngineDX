#pragma once

#include <span>
#include "Mesh.h"
#include "BasicMaterial.h"

namespace tinygltf { class Model; }

class Model
{
public:

    Model();
    ~Model();

    void load(const char* fileName, const char* basePath);

    uint32_t getNumMeshes() const { return numMeshes; }
    uint32_t getNumMaterials() const { return numMaterials;  }

    std::span<const Mesh> getMeshes() const { return std::span<const Mesh>(meshes.get(), numMeshes); }
    std::span<const BasicMaterial> getMaterials() const { return std::span<const BasicMaterial>(materials.get(), numMaterials); }

    const Matrix& getMatrix() const { return matrix; }
    void setMatrix(const Matrix& m) { matrix = m; }

    const std::string& getSrcFile() const { return srcFile; }

private:

    void loadMeshes(const tinygltf::Model& model);
    void loadMaterials(const tinygltf::Model& model, const char* basePath);

private:
    struct TextureInfo
    {
        ComPtr<ID3D12Resource> resource;
        UINT desc = 0;
        Vector4 colour;
    };

    Matrix matrix;
    std::unique_ptr<BasicMaterial[]> materials;
    std::unique_ptr<Mesh[]> meshes;
    uint32_t numMeshes = 0;
    uint32_t numMaterials = 0;
    std::string srcFile;
};
