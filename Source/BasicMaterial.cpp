#include "Globals.h"
#include "BasicMaterial.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleDescriptors.h"

#include "tiny_gltf.h"

#include <imgui.h>


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
    }
    else if(materialType == PHONG)
    {
        materialData.phong.diffuseColour = baseColour;
        materialData.phong.Kd = 0.85f;
        materialData.phong.Ks = 0.35f;
        materialData.phong.shininess = 32.0f;
        materialData.phong.hasDiffuseTex = hasColourTexture;
    }
    else if (materialType == PBR_PHONG)
    {
        materialData.pbrPhong.diffuseColour = XMFLOAT3(baseColour.x, baseColour.y, baseColour.z);
        materialData.pbrPhong.hasDiffuseTex = hasColourTexture;
        materialData.pbrPhong.shininess = 32.0f;
        materialData.pbrPhong.specularColour = XMFLOAT3(0.0f, 0.0f, 0.0f);
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
}

void BasicMaterial::setPhongMaterial(const PhongMaterialData& phong)
{
    materialData.phong = phong;
    if (!baseColourTex)
    {
        materialData.phong.hasDiffuseTex = FALSE;
    }
}

void BasicMaterial::setPBRPhongMaterial(const PBRPhongMaterialData& pbr)
{
    materialData.pbrPhong = pbr;
    if (!baseColourTex)
    {
        materialData.pbrPhong.hasDiffuseTex = FALSE;
    }
}


