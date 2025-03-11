#pragma once

#include "DescriptorHeaps.h"

class Mesh;

namespace tinygltf { class Model; }

class Model
{
public:
    Model();
    ~Model();

    void load(const char* fileName);

private:
    void loadMeshes(const tinygltf::Model& model);
    void loadMaterials(const tinygltf::Model& model);

private:
    struct TextureInfo
    {
        ComPtr<ID3D12Resource> resource;
        UINT desc = UINT32_MAX;
    };

    std::unique_ptr<TextureInfo[]> textures;
    std::unique_ptr<Mesh[]> meshes;
};
