#include "Globals.h"
#include "BasicMaterial.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "TableDescriptors.h"
#include "SingleDescriptors.h"

#include "tiny_gltf.h"

#include <imgui.h>


BasicMaterial::BasicMaterial()
{

}

BasicMaterial::~BasicMaterial()
{
    app->getShaderDescriptors()->getTable()->release(textureTableSRV);
}

void BasicMaterial::load(const tinygltf::Model& model, const tinygltf::Material& material, Type type, const char* basePath)
{
    name = material.name;

    Vector4 baseColour = Vector4(float(material.pbrMetallicRoughness.baseColorFactor[0]),
                                 float(material.pbrMetallicRoughness.baseColorFactor[1]),
                                 float(material.pbrMetallicRoughness.baseColorFactor[2]),
                                 float(material.pbrMetallicRoughness.baseColorFactor[3]));

    BOOL hasColourTexture = FALSE, hasMetallicRoughnessTex = FALSE;

    auto loadTexture = [](int index, const tinygltf::Model& model, const char* basePath, ComPtr<ID3D12Resource>& outTex) -> BOOL
    {
        if (index < 0 || index >= int(model.textures.size()))
            return FALSE;

        const tinygltf::Texture& texture = model.textures[index];
        const tinygltf::Image& image = model.images[texture.source];
        if (!image.uri.empty())
        {
            outTex = app->getResources()->createTextureFromFile(std::string(basePath) + image.uri);
            return TRUE;
        }
        return FALSE;
    };

    hasColourTexture = loadTexture(material.pbrMetallicRoughness.baseColorTexture.index, model, basePath, baseColourTex);
    hasMetallicRoughnessTex = materialType == METALLIC_ROUGHNESS && loadTexture(material.pbrMetallicRoughness.metallicRoughnessTexture.index, model, basePath, metallicRoughnessTex);

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
        materialData.pbrPhong.shininess = 64.0f;
        materialData.pbrPhong.specularColour = XMFLOAT3(0.015f, 0.015f, 0.015f);
    }
    else if(materialType == METALLIC_ROUGHNESS)
    {
        materialData.metallicRoughness.baseColour = baseColour;
        materialData.metallicRoughness.metallicFactor = float(material.pbrMetallicRoughness.metallicFactor);
        materialData.metallicRoughness.roughnessFactor = float(material.pbrMetallicRoughness.roughnessFactor);
        materialData.metallicRoughness.hasBaseColourTex = hasColourTexture;
        materialData.metallicRoughness.hasMetallicRoughnessTex = hasMetallicRoughnessTex;
    }

    // Descriptors 

    TableDescriptors* descriptors = app->getShaderDescriptors()->getTable();
    textureTableSRV = descriptors->alloc();

    if (hasColourTexture)
    {
        descriptors->createTextureSRV(baseColourTex.Get(), textureTableSRV, 0);
    }
    else
    {
        descriptors->createNullTexture2DSRV(textureTableSRV, 0);
    }

    if (hasMetallicRoughnessTex)
    {
        descriptors->createTextureSRV(metallicRoughnessTex.Get(), textureTableSRV, 1);
    }
    else
    {
        descriptors->createNullTexture2DSRV(textureTableSRV, 1);
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

void BasicMaterial::setMetallicRoughnessMaterial(const MetallicRoughnessMaterialData& pbr)
{
    materialData.metallicRoughness = pbr;
    if (!baseColourTex)
    {
        materialData.metallicRoughness.hasBaseColourTex = FALSE;
    }
}

