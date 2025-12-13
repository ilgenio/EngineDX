#include "Globals.h"
#include "Material.h"

#include "Application.h"
#include "ModuleTextureManager.h"
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
    data.baseColor       = Vector4(pow(float(material.pbrMetallicRoughness.baseColorFactor[0]), 2.2f), 
                                   pow(float(material.pbrMetallicRoughness.baseColorFactor[1]), 2.2f), 
                                   pow(float(material.pbrMetallicRoughness.baseColorFactor[2]), 2.2f), 
                                       float(material.pbrMetallicRoughness.baseColorFactor[3]));
    data.metallicFactor  = float(material.pbrMetallicRoughness.metallicFactor);
    data.roughnessFactor = float(material.pbrMetallicRoughness.roughnessFactor);
    data.emissiveFactor = material.emissiveFactor.size() >= 3 ? Vector3(float(material.emissiveFactor[0]),
                                                                        float(material.emissiveFactor[1]),
                                                                        float(material.emissiveFactor[2])) : Vector3::Zero;
    data.flags           = 0;

    std::string base = basePath;

    // Descriptors
    textureTableDesc = app->getShaderDescriptors()->allocTable();
     
    // Base Color Texture
    if (material.pbrMetallicRoughness.baseColorTexture.index >= 0 && loadTexture(model, base, material.pbrMetallicRoughness.baseColorTexture.index, true, textures[TEX_SLOT_BASECOLOUR]))
    {
        data.flags |= FLAG_HAS_BASECOLOUR_TEX;
        textureTableDesc.createTextureSRV(textures[TEX_SLOT_BASECOLOUR].texture.Get(), TEX_SLOT_BASECOLOUR);
    }
    else
    {
        textureTableDesc.createNullTexture2DSRV(TEX_SLOT_BASECOLOUR);
    }

    // Metallic Roughness Texture
    if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0 && loadTexture(model, base, material.pbrMetallicRoughness.metallicRoughnessTexture.index, false, textures[TEX_SLOT_METALLIC_ROUGHNESS]))
    {
        data.flags |= FLAG_HAS_METALLICROUGHNESS_TEX;
        textureTableDesc.createTextureSRV(textures[TEX_SLOT_METALLIC_ROUGHNESS].texture.Get(), TEX_SLOT_METALLIC_ROUGHNESS);
    }
    else
    {
        textureTableDesc.createNullTexture2DSRV(TEX_SLOT_METALLIC_ROUGHNESS);
    }

    // Normals Texture
    if (material.normalTexture.index >= 0 && loadTexture(model, base, material.normalTexture.index, false, textures[TEX_SLOT_NORMAL]))
    {
        data.flags |= FLAG_HAS_NORMAL_TEX;

        DXGI_FORMAT format = textures[TEX_SLOT_NORMAL].texture->GetDesc().Format;
        data.flags |= (format == DXGI_FORMAT_BC5_TYPELESS || format == DXGI_FORMAT_BC5_UNORM || format == DXGI_FORMAT_BC5_SNORM) ? FLAG_HAS_COMPRESSED_NORMAL : 0;

        textureTableDesc.createTextureSRV(textures[TEX_SLOT_NORMAL].texture.Get(), TEX_SLOT_NORMAL);
    }
    else
    {
        textureTableDesc.createNullTexture2DSRV(TEX_SLOT_NORMAL);
    }

    data.normalScale = float(material.normalTexture.scale);

    // Occlusion Texture
    if (material.occlusionTexture.index >= 0 && loadTexture(model, base, material.occlusionTexture.index, false, textures[TEX_SLOT_OCCLUSION]))
    {
        data.flags |= FLAG_HAS_OCCLUSION_TEX;
        textureTableDesc.createTextureSRV(textures[TEX_SLOT_OCCLUSION].texture.Get(), TEX_SLOT_OCCLUSION);
    }
    else
    {
        textureTableDesc.createNullTexture2DSRV(TEX_SLOT_OCCLUSION);
    }

    data.occlusionStrength = float(material.occlusionTexture.strength);

    if(material.emissiveTexture.index >= 0 && loadTexture(model, base, material.emissiveTexture.index, true, textures[TEX_SLOT_EMISSIVE]))
    {
        data.flags |= FLAG_HAS_EMISSIVE_TEX;
        textureTableDesc.createTextureSRV(textures[TEX_SLOT_EMISSIVE].texture.Get(), TEX_SLOT_EMISSIVE);
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


bool Material::loadTexture(const tinygltf::Model& model, const std::string& basePath, int index, bool defaultSRGB, TextureInfo& output)
{
    const tinygltf::Texture& texture = model.textures[index];
    const tinygltf::Image& image = model.images[texture.source];

    if (image.mimeType.empty())
    {
        ModuleTextureManager* textureManager = app->getTextureManager();
        
        output.path = textureManager->getNormalizedPath(basePath + image.uri);
        output.texture = textureManager->createTexture(output.path, defaultSRGB);

        _ASSERT_EXPR(output.texture, L"Can't load texture");

        return output.texture;
    }

    return false;
};
