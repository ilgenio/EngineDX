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

    void load(const char* fileName, const char* basePath, BasicMaterial::Type materialType);

    uint32_t getNumMeshes() const { return numMeshes; }
    uint32_t getNumMaterials() const { return numMaterials;  }

    std::span<const Mesh> getMeshes() const { return std::span<const Mesh>(meshes.get(), numMeshes); }
    std::span<const BasicMaterial> getMaterials() const { return std::span<const BasicMaterial>(materials.get(), numMaterials); }
    std::span<BasicMaterial> getMaterials() { return std::span<BasicMaterial>(materials.get(), numMaterials); }

    const Matrix& getModelMatrix() const { return matrix; }
    void setModelMatrix(const Matrix& m) { matrix = m; }
    Matrix getNormalMatrix() const 
    {
        Matrix normal = matrix;
        normal.Translation(Vector3::Zero);
        normal.Invert();
        normal.Transpose();

        return normal;
    }

    const std::string& getSrcFile() const { return srcFile; }

private:

    void loadMeshes(const tinygltf::Model& model);
    void loadMaterials(const tinygltf::Model& model, const char* basePath, BasicMaterial::Type materialType);

private:

    struct TextureInfo
    {
        ComPtr<ID3D12Resource> resource;
        UINT desc = 0;
        Vector4 colour;
    };

    struct Transforms
    {
        XMFLOAT4X4 model;
        XMFLOAT3X3 normal;
    };

    Matrix matrix = Matrix::Identity;
    std::unique_ptr<BasicMaterial[]> materials;
    std::unique_ptr<Mesh[]> meshes;
    uint32_t numMeshes = 0;
    uint32_t numMaterials = 0;
    std::string srcFile;

    ComPtr<ID3D12Resource> transformBuffer;
};
