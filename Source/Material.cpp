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
    if (material.pbrMetallicRoughness.baseColorTexture.index >= 0 && loadTexture(model, base, material.pbrMetallicRoughness.baseColorTexture.index, baseColorTex))
    {
        data.flags |= FLAG_HAS_BASECOLOUR_TEX;
        textureTableDesc.createTextureSRV(baseColorTex.Get(), TEX_SLOT_BASECOLOUR);
    }

    // Metallic Roughness Texture
    if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0 && loadTexture(model, base, material.pbrMetallicRoughness.metallicRoughnessTexture.index, metRougTex))
    {
        data.flags |= FLAG_HAS_METALLICROUGHNESS_TEX;
        textureTableDesc.createTextureSRV(metRougTex.Get(), TEX_SLOT_METALLIC_ROUGHNESS);
    }

    // Normals Texture
    if (material.normalTexture.index >= 0 && loadTexture(model, base, material.normalTexture.index, normalTex))
    {
        textureTableDesc.createTextureSRV(normalTex.Get(), TEX_SLOT_NORMAL);
    }

    data.normalScale = float(material.normalTexture.scale);

    // Occlusion Texture
    if (material.occlusionTexture.index >= 0 && loadTexture(model, base, material.occlusionTexture.index, occlusionTex))
    {
        textureTableDesc.createTextureSRV(occlusionTex.Get(), TEX_SLOT_OCCLUSION);
    }

    // \todo: occlusion strength

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

    // Material Buffer
}


bool Material::loadTexture(const tinygltf::Model& model, const std::string& basePath, int index, ComPtr<ID3D12Resource>& output)
{
    const tinygltf::Texture& texture = model.textures[index];
    const tinygltf::Image& image = model.images[texture.source];

    if (image.mimeType.empty())
    {
        output = app->getResources()->createTextureFromFile(basePath + image.uri);

        return true;
    }

    return false;
};
