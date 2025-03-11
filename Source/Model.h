#pragma once

#include "DescriptorHeaps.h"

class Mesh;

namespace tinygltf { class Model; }

class Model
{
public:
    Model();
    ~Model();

    void load(const char* fileName, const char* basePath);

    uint32_t getNumMeshes() const { return numMeshes; }
    uint32_t getNumTextures() const { return numTextures;  }

    const Mesh& getMesh(uint32_t index) const { _ASSERTE(index < numMeshes); return meshes[index]; }
    UINT getTextureDescriptor(uint32_t index) const { _ASSERTE(index < numTextures); return textures[index].desc; }

private:
    void loadMeshes(const tinygltf::Model& model);
    void loadMaterials(const tinygltf::Model& model, const char* basePath);

private:
    struct TextureInfo
    {
        ComPtr<ID3D12Resource> resource;
        UINT desc = 0;
    };

    std::unique_ptr<TextureInfo[]> textures;
    std::unique_ptr<Mesh[]> meshes;
    uint32_t numMeshes = 0;
    uint32_t numTextures = 0;
};
