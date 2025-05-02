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

void BasicMaterial::load(const tinygltf::Model& model, const tinygltf::Material& material, Type type, const char* basePath)
{
    name = material.name;

    Vector4 baseColour = Vector4(float(material.pbrMetallicRoughness.baseColorFactor[0]),
                                 float(material.pbrMetallicRoughness.baseColorFactor[1]),
                                 float(material.pbrMetallicRoughness.baseColorFactor[2]),
                                 float(material.pbrMetallicRoughness.baseColorFactor[3]));

    BOOL hasColourTexture = FALSE;
    int textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;

    if (textureIndex >= 0)
    {
        const tinygltf::Texture& texture = model.textures[textureIndex];
        const tinygltf::Image& image = model.images[texture.source];

        if (!image.uri.empty())
        {
            baseColourTex = app->getResources()->createTextureFromFile(std::string(basePath) + image.uri);

            hasColourTexture = TRUE;
        }
    }

    materialType = type;

    if (materialType == BASIC)
    {
        materialData.basic.baseColour = baseColour;
        materialData.basic.hasColourTexture = hasColourTexture;

        materialBuffer = app->getResources()->createUploadBuffer(&materialData.basic, alignUp(sizeof(BasicData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT), name.c_str());
    }
    else if(materialType == PHONG)
    {
        materialData.phong.diffuseColour = baseColour;
        materialData.phong.Kd = 1.0;
        materialData.phong.Ks = 0.0;
        materialData.phong.shininess = 0.0;
        materialData.phong.hasDiffuseTex = hasColourTexture;

        materialBuffer = app->getResources()->createUploadBuffer(&materialData.phong, alignUp(sizeof(PhongData), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT), name.c_str());
    }

    // Descriptors 

    if (hasColourTexture)
    {
        baseColourSRV = app->getDescriptors()->createTextureSRV(baseColourTex.Get());
    }
    else
    {
        baseColourSRV = app->getDescriptors()->createNullTexture2DSRV();
    }
 
    materialCBV = app->getDescriptors()->createCBV(materialBuffer.Get());
}


