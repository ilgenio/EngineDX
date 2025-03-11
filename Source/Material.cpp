#include "Globals.h"
#include "Material.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleDescriptors.h"
#include "ModuleD3D12.h"

#include "tiny_gltf.h"
#include <filesystem>
#include <algorithm>

ComPtr<ID3D12RootSignature> Material::rootSignature;

Material::Material()
{
}

Material::~Material()
{
}

void Material::createSharedData()
{
    CD3DX12_ROOT_PARAMETER rootParameters[4];

    // Model matrix
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[0].Constants.Num32BitValues = (sizeof(Matrix) / sizeof(UINT32));
    rootParameters[0].Constants.RegisterSpace = 0;
    rootParameters[0].Constants.ShaderRegister = 0;

    // Camera CBV
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[1].Descriptor.RegisterSpace = 0;
    rootParameters[1].Descriptor.ShaderRegister = 1;

    // Material CBV
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[2].Descriptor.RegisterSpace = 0;
    rootParameters[2].Descriptor.ShaderRegister = 2;

    // Material Textures

    D3D12_DESCRIPTOR_RANGE tableRange;
    tableRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    tableRange.NumDescriptors = 6;
    tableRange.BaseShaderRegister = 0;
    tableRange.RegisterSpace = 0;
    tableRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[3].DescriptorTable.pDescriptorRanges = &tableRange;

    // Samplers
    D3D12_STATIC_SAMPLER_DESC linearClampSampler;
    linearClampSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    linearClampSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    linearClampSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    linearClampSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    linearClampSampler.MipLODBias = 0;
    linearClampSampler.MaxAnisotropy = 0;
    linearClampSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    linearClampSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    linearClampSampler.MinLOD = 0.0f;
    linearClampSampler.MaxLOD = D3D12_FLOAT32_MAX;
    linearClampSampler.ShaderRegister = 0;
    linearClampSampler.RegisterSpace = 0;
    linearClampSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;

    rootSignatureDesc.Init(UINT(std::size(rootParameters)), rootParameters, 1, &linearClampSampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootSignatureBlob;

    D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, nullptr);
    app->getD3D12()->getDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
}

void Material::destroySharedData()
{
    rootSignature.Reset();
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

    std::string base = basePath;

    // Descriptors
    ModuleDescriptors* descriptors = app->getDescriptors();

    // Base Color Texture
    if (material.pbrMetallicRoughness.baseColorTexture.index >= 0 && loadTexture(model, base, material.pbrMetallicRoughness.baseColorTexture.index, baseColorTex))
    {
        baseColourDesc = descriptors->createTextureSRV(baseColorTex.Get());
    }

    // Metallic Roughness Texture
    if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0 && loadTexture(model, base, material.pbrMetallicRoughness.metallicRoughnessTexture.index, metRougTex))
    {
        metalRoughDesc = descriptors->createTextureSRV(metRougTex.Get());
    }

    // Normals Texture
    if (material.normalTexture.index >= 0 && loadTexture(model, base, material.normalTexture.index, normalTex))
    {
        normalDesc = descriptors->createTextureSRV(normalTex.Get());
    }

    data.normalScale = float(material.normalTexture.scale);

    // Occlusion Texture
    if (material.occlusionTexture.index >= 0 && loadTexture(model, base, material.occlusionTexture.index, occlusionTex))
    {
        occlusionDesc = descriptors->createTextureSRV(occlusionTex.Get());
    }

    // \todo: occlusion strength

    if (material.alphaMode == "MASK")
    {
        alphaMode = ALPHA_MODE_MASK;
        alphaCutoff = float(material.alphaCutoff);
    }
    else if (material.alphaMode == "BLEND")
    {
        alphaMode = ALPHA_MODE_BLEND;
    }
    else
    {
        alphaMode = ALPHA_MODE_OPAQUE;
    }

    loadIridescenceExt(model, material, base);
    loadTransmissionExt(model, material, base);
    loadIORExt(material);

    // Material Buffer

    materialBuffer = app->getResources()->createDefaultBuffer(&data, sizeof(data), name.c_str(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    materialDesc = descriptors->createCBV(materialBuffer.Get());
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

void Material::loadIridescenceExt(const tinygltf::Model& model, const tinygltf::Material &material, const std::string& basePath)
{
    ModuleDescriptors* descriptors = app->getDescriptors();

    auto it = material.extensions.find("KHR_materials_iridescence");
    if (it != material.extensions.end())
    {
        data.flags |= MATERIAL_FLAGS_IRIDESCENCE;

        const tinygltf::Value& iridescenceMap = it->second;

        const tinygltf::Value& factor = iridescenceMap.Get(std::string("iridescenceFactor"));
        if (factor.IsNumber())
        {
            data.iridescence.factor = float(factor.GetNumberAsDouble());
        }

        const tinygltf::Value& ior = iridescenceMap.Get(std::string("iridescenceIor"));
        if (ior.IsNumber())
        {
            data.iridescence.IOR = float(ior.GetNumberAsDouble());
        }

        const tinygltf::Value& maxThick = iridescenceMap.Get(std::string("iridescenceThicknessMaximum"));
        if(maxThick.IsNumber())
        {
            data.iridescence.maxThikness = float(maxThick.GetNumberAsDouble());
        }

        const tinygltf::Value& minThick = iridescenceMap.Get(std::string("iridescenceThicknessMinimum"));
        if(minThick.IsNumber())
        {
            data.iridescence.minThikness = float(minThick.GetNumberAsDouble());
        }

        const tinygltf::Value& texture = iridescenceMap.Get(std::string("iridescenceTexture"));
        if(texture.Type() != tinygltf::NULL_TYPE)
        {
            int index = texture.Get(std::string("index")).GetNumberAsInt();
            if (index >= 0 && loadTexture(model, basePath, index, iridescenceTex))
            {
                iridiscenceDesc = descriptors->createTextureSRV(iridescenceTex.Get());
                data.flags |= MATERIAL_FLAGS_IRIDESCENCE_TEX;
            }
        }

        const tinygltf::Value& thickTexture = iridescenceMap.Get(std::string("iridescenceThicknessTexture"));
        if(thickTexture.IsObject())
        {
            int index = thickTexture.Get(std::string("index")).GetNumberAsInt();
            if (index >= 0 && loadTexture(model, basePath, index, iridescenceThicknessTex))
            {
                iridiscenceThicknessDesc = descriptors->createTextureSRV(iridescenceThicknessTex.Get());
                data.flags |= MATERIAL_FLAGS_IRIDESCENCE_THICKNESS_TEX;
            }
        }
    }
}

void Material::loadTransmissionExt(const tinygltf::Model &model, const tinygltf::Material &material, const std::string &basePath)
{
    auto it = material.extensions.find("KHR_materials_transmission");
    if (it != material.extensions.end())
    {
        data.flags |= MATERIAL_FLAGS_TRANSMISSION;

        const tinygltf::Value& iridescenceMap = it->second;

        const tinygltf::Value& factor = iridescenceMap.Get(std::string("transmissionFactor"));
        if (factor.IsNumber())
        {
            data.transmissionFactor = float(factor.GetNumberAsDouble());
        }

        const tinygltf::Value& texture = iridescenceMap.Get(std::string("transmissionTexture"));
        if(texture.Type() != tinygltf::NULL_TYPE)
        {
            int index = texture.Get(std::string("index")).GetNumberAsInt();
            if (index >= 0 && loadTexture(model, basePath, index, transmissionTex))
            {
                transmissionDesc = app->getDescriptors()->createTextureSRV(transmissionTex.Get());
                data.flags |= MATERIAL_FLAGS_TRANSMISSION_TEX;
            }
        }
    }
}

void Material::loadIORExt(const tinygltf::Material &material)
{
    auto it = material.extensions.find("KHR_materials_ior");
    if(it != material.extensions.end())
    {
        const tinygltf::Value& iorMap = it->second;
        const tinygltf::Value& ior = iorMap.Get(std::string("ior"));
        if(ior.IsNumber())
        {
            float iorValue = float(ior.GetNumberAsDouble());
            data.dielectricF0 = (iorValue-1)/(iorValue+1);
            data.dielectricF0 *= data.dielectricF0;
        }
    }
}

