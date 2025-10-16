#include "Globals.h"
#include "BasicMaterial.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 

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
    materialType = type;

    Vector4 baseColour = Vector4(float(material.pbrMetallicRoughness.baseColorFactor[0]),
                                 float(material.pbrMetallicRoughness.baseColorFactor[1]),
                                 float(material.pbrMetallicRoughness.baseColorFactor[2]),
                                 float(material.pbrMetallicRoughness.baseColorFactor[3]));

    BOOL hasColourTexture = FALSE, hasMetallicRoughnessTex = FALSE, hasOcclusionTex = FALSE, hasNormalTex = FALSE, hasEmissiveTex = FALSE;

    auto loadTexture = [](int index, const tinygltf::Model& model, const char* basePath, bool defaultSRGB, ComPtr<ID3D12Resource>& outTex) -> BOOL
    {
        if (index < 0 || index >= int(model.textures.size()))
            return FALSE;

        const tinygltf::Texture& texture = model.textures[index];
        const tinygltf::Image& image = model.images[texture.source];
        if (!image.uri.empty())
        {
            outTex = app->getResources()->createTextureFromFile(std::string(basePath) + image.uri, defaultSRGB);
            return TRUE;
        }
        return FALSE;
    };

    hasColourTexture = loadTexture(material.pbrMetallicRoughness.baseColorTexture.index, model, basePath, true, baseColourTex);
    hasMetallicRoughnessTex = materialType == METALLIC_ROUGHNESS && loadTexture(material.pbrMetallicRoughness.metallicRoughnessTexture.index, model, basePath, false, metallicRoughnessTex);
    hasOcclusionTex = materialType == METALLIC_ROUGHNESS && loadTexture(material.occlusionTexture.index, model, basePath, false, occlusionTex);
    hasNormalTex = materialType == METALLIC_ROUGHNESS && loadTexture(material.normalTexture.index, model, basePath, false, normalTex);
    hasEmissiveTex = materialType == METALLIC_ROUGHNESS && loadTexture(material.emissiveTexture.index, model, basePath, true, emissiveTex);

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
        Vector3 emissiveFactor = material.emissiveFactor.size() >=3 ? Vector3(float(material.emissiveFactor[0]),
            float(material.emissiveFactor[1]),
            float(material.emissiveFactor[2])) : Vector3::Zero;

        materialData.metallicRoughness.baseColour = baseColour;
        materialData.metallicRoughness.metallicFactor = float(material.pbrMetallicRoughness.metallicFactor);
        materialData.metallicRoughness.roughnessFactor = float(material.pbrMetallicRoughness.roughnessFactor);
        materialData.metallicRoughness.hasBaseColourTex = hasColourTexture;
        materialData.metallicRoughness.hasMetallicRoughnessTex = hasMetallicRoughnessTex;
        materialData.metallicRoughness.occlusionStrength = float(material.occlusionTexture.strength);
        materialData.metallicRoughness.emissiveFactor = emissiveFactor;
        materialData.metallicRoughness.normalScale = float(material.normalTexture.scale);
        materialData.metallicRoughness.hasOcclusionTex = hasOcclusionTex;
        materialData.metallicRoughness.hasEmissive = hasEmissiveTex;
        materialData.metallicRoughness.hasNormalMap = hasNormalTex;
    }

    // Descriptors 

    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
    textureTableDesc = descriptors->allocTable();

    if (hasColourTexture)
    {
        textureTableDesc.createTextureSRV(baseColourTex.Get(), 0);
    }
    else
    {
        textureTableDesc.createNullTexture2DSRV(0);
    }

    if (hasMetallicRoughnessTex)
    {
        textureTableDesc.createTextureSRV(metallicRoughnessTex.Get(), 1);
    }
    else
    {
        textureTableDesc.createNullTexture2DSRV(1);
    }

    if( hasOcclusionTex)
    {
        textureTableDesc.createTextureSRV(occlusionTex.Get(), 2);
    }
    else
    {
        textureTableDesc.createNullTexture2DSRV(2);
    }   

    if(hasEmissiveTex)
    {
        textureTableDesc.createTextureSRV(emissiveTex.Get(), 3);
    }
    else
    {
        textureTableDesc.createNullTexture2DSRV(3);
    }

    if(hasNormalTex)
    {
        textureTableDesc.createTextureSRV(normalTex.Get(), 4);
    }
    else
    {
        textureTableDesc.createNullTexture2DSRV(4);
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

