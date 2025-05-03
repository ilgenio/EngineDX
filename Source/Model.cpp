#include "Globals.h"
#include "Model.h"

#include "Application.h"
#include "ModuleDescriptors.h"
#include "ModuleResources.h"

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

void Model::load(const char* fileName, const char* basePath, BasicMaterial::Type materialType)
{
    tinygltf::TinyGLTF gltfContext;
    tinygltf::Model model;
    std::string error, warning;

    bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, fileName);

    if (loadOk)
    {
        srcFile = fileName;
        loadMaterials(model, basePath, materialType);
        loadMeshes(model);
    }
    else
    {
        LOG("Error loading %s: %s", fileName, error.c_str());
    }
}

void Model::loadMeshes(const tinygltf::Model& model)
{
    numMeshes = uint32_t(model.meshes.size());
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

void Model::loadMaterials(const tinygltf::Model& model, const char* basePath, BasicMaterial::Type materialType)
{
    ModuleDescriptors* descriptors = app->getDescriptors();
    ModuleResources* resources = app->getResources();

    numMaterials = uint32_t(model.materials.size());
    materials = std::make_unique<BasicMaterial[]>(numMaterials);
    int materialIndex = 0;

    for(const tinygltf::Material& material : model.materials)
    {
        materials[materialIndex++].load(model, material, materialType, basePath);
    }
}

