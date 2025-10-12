#include "Globals.h"
#include "Material.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleD3D12.h"

#include "tiny_gltf.h"


Material::Material()
{
}

Material::~Material()
{
}

void Material::load(const tinygltf::Model& model, const tinygltf::Material &material, const char* basePath)
{
    ModuleResources* resources = app->getResources();
    name                 = material.name;
    data.baseColor       = Vector4(float(material.pbrMetallicRoughness.baseColorFactor[0]), 
                                  float(material.pbrMetallicRoughness.baseColorFactor[1]), 
                                  float(material.pbrMetallicRoughness.baseColorFactor[2]), 
                                  float(material.pbrMetallicRoughness.baseColorFactor[3]));
    data.metallicFactor  = float(material.pbrMetallicRoughness.metallicFactor);
    data.roughnessFactor = float(material.pbrMetallicRoughness.roughnessFactor);
    data.flags           = 0;

    std::string base = basePath;

    // Descriptors
    textureTableDesc = app->getShaderDescriptors()->allocTable();

    // Base Color Texture
    if (material.pbrMetallicRoughness.baseColorTexture.index >= 0 && loadTexture(model, base, material.pbrMetallicRoughness.baseColorTexture.index, true, baseColorTex))
    {
        _ASSERT_EXPR(baseColorTex, "Can't load base color texture");

        data.flags |= FLAG_HAS_BASECOLOUR_TEX;
        textureTableDesc.createTextureSRV(baseColorTex.Get(), TEX_SLOT_BASECOLOUR);
    }
    else
    {
        textureTableDesc.createNullTexture2DSRV(TEX_SLOT_BASECOLOUR);
    }

    // Metallic Roughness Texture
    if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0 && loadTexture(model, base, material.pbrMetallicRoughness.metallicRoughnessTexture.index, false, metRougTex))
    {
        _ASSERT_EXPR(metRougTex, "Can't load metallic roughness texture");

        data.flags |= FLAG_HAS_METALLICROUGHNESS_TEX;
        textureTableDesc.createTextureSRV(metRougTex.Get(), TEX_SLOT_METALLIC_ROUGHNESS);
    }
    else
    {
        textureTableDesc.createNullTexture2DSRV(TEX_SLOT_METALLIC_ROUGHNESS);
    }

    // Normals Texture
    if (material.normalTexture.index >= 0 && loadTexture(model, base, material.normalTexture.index, false, normalTex))
    {
        data.flags |= FLAG_HAS_NORMAL_TEX;
        textureTableDesc.createTextureSRV(normalTex.Get(), TEX_SLOT_NORMAL);
    }
    else
    {
        textureTableDesc.createNullTexture2DSRV(TEX_SLOT_NORMAL);
    }

    data.normalScale = float(material.normalTexture.scale);

    // Occlusion Texture
    if (material.occlusionTexture.index >= 0 && loadTexture(model, base, material.occlusionTexture.index, false, occlusionTex))
    {
        _ASSERT_EXPR(occlusionTex, "Can't load occlusion texture");

        data.flags |= FLAG_HAS_OCCLUSION_TEX;
        textureTableDesc.createTextureSRV(occlusionTex.Get(), TEX_SLOT_OCCLUSION);
    }
    else
    {
        textureTableDesc.createNullTexture2DSRV(TEX_SLOT_OCCLUSION);
    }

    data.occlusionStrength = float(material.occlusionTexture.strength);

    if(material.emissiveTexture.index >= 0 && loadTexture(model, base, material.emissiveTexture.index, true, emissiveTex))
    {
        data.flags |= FLAG_HAS_EMISSIVE_TEX;
        textureTableDesc.createTextureSRV(emissiveTex.Get(), TEX_SLOT_EMISSIVE);
    }
    else
    {
        textureTableDesc.createNullTexture2DSRV(TEX_SLOT_EMISSIVE);
    }

    if (material.alphaMode == "BLEND")
    {
        alphaMode = ALPHA_MODE_BLEND;
    }
    else
    {
        alphaMode = ALPHA_MODE_OPAQUE;

        if (material.alphaMode == "MASK")
        {
            data.alphaCutoff = material.alphaMode == "MASK" ? float(material.alphaCutoff) : 0.0f;
        }
    }
}


bool Material::loadTexture(const tinygltf::Model& model, const std::string& basePath, int index, bool defaultSRGB, ComPtr<ID3D12Resource>& output)
{
    const tinygltf::Texture& texture = model.textures[index];
    const tinygltf::Image& image = model.images[texture.source];

    if (image.mimeType.empty())
    {
        output = app->getResources()->createTextureFromFile(basePath + image.uri, defaultSRGB);

        _ASSERT_EXPR(output, "Can't load texture");

        return true;
    }

    return false;
};
