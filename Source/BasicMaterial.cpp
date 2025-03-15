#include "Globals.h"
#include "BasicMaterial.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleDescriptors.h"

#include "tiny_gltf.h"


BasicMaterial::BasicMaterial()
{

}

BasicMaterial::~BasicMaterial()
{
}

void BasicMaterial::load(const tinygltf::Model& model, const tinygltf::Material& material, const char* basePath)
{
    name = material.name;
    materialData.baseColour = Vector4(float(material.pbrMetallicRoughness.baseColorFactor[0]),
                                      float(material.pbrMetallicRoughness.baseColorFactor[1]),
                                      float(material.pbrMetallicRoughness.baseColorFactor[2]),
                                      float(material.pbrMetallicRoughness.baseColorFactor[3]));

    materialData.hasColourTexture = 0;
    int textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;

    if (textureIndex >= 0)
    {
        const tinygltf::Texture& texture = model.textures[textureIndex];
        const tinygltf::Image& image = model.images[texture.source];

        if (!image.uri.empty())
        {
            baseColourTex = app->getResources()->createTextureFromFile(std::string(basePath) + image.uri);

            materialData.hasColourTexture = 1;
        }
    }

    if (materialData.hasColourTexture)
    {
        baseColourSRV = app->getDescriptors()->createTextureSRV(baseColourTex.Get());
    }

    materialBuffer = app->getResources()->createDefaultBuffer(&materialData, sizeof(MaterialData), name.c_str(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    materialCBV = app->getDescriptors()->createCBV(materialBuffer.Get());
}


