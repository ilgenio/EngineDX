#include "Globals.h"
#include "Model.h"

#include "Application.h"
#include "ModuleDescriptors.h"
#include "ModuleResources.h"
#include "Mesh.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#define TINYGLTF_IMPLEMENTATION /* Only in one of the includes */
#include "tiny_gltf.h"


Model::Model()
{
}

Model::~Model()
{
}

void Model::load(const char* fileName, const char* basePath)
{
    tinygltf::TinyGLTF gltfContext;
    tinygltf::Model model;
    std::string error, warning;

    bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, fileName);

    if (loadOk)
    {
        loadMaterials(model, basePath);
        loadMeshes(model);
    }
    else
    {
        LOG("Error loading %s: %s", fileName, error.c_str());
    }

}

void Model::loadMeshes(const tinygltf::Model& model)
{
    numMeshes = model.meshes.size();
    meshes = std::make_unique<Mesh[]>(numMeshes);
    int meshIndex = 0;

    for(const tinygltf::Mesh& mesh : model.meshes)
    {
        for(const tinygltf::Primitive& primitive : mesh.primitives)
        {
            meshes[meshIndex++].load(model, mesh, primitive);
        }
    }
}

void Model::loadMaterials(const tinygltf::Model& model, const char* basePath)
{
    ModuleDescriptors* descriptors = app->getDescriptors();
    ModuleResources* resources = app->getResources();

    numTextures = model.materials.size();
    textures = std::make_unique<TextureInfo[]>(numTextures);
    int materialIndex = 0;

    for(const tinygltf::Material& material : model.materials)
    {
        int textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
        if (textureIndex >= 0)
        {
            const tinygltf::Texture& texture = model.textures[textureIndex];
            const tinygltf::Image& image = model.images[texture.source];

            if (!image.uri.empty())
            {
                textures[materialIndex].resource = resources->createTextureFromFile(std::string(basePath)+image.uri);
                textures[materialIndex].desc = descriptors->createTextureSRV(textures[materialIndex].resource.Get());
            }
        }

        ++materialIndex;
    }
}

