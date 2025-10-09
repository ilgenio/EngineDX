#pragma once

#include "ShaderTableDesc.h"

namespace tinygltf { class Model; struct Material;  }

struct BasicMaterialData
{
    XMFLOAT4 baseColour;
    BOOL     hasColourTexture;
};

struct PhongMaterialData
{
    XMFLOAT4 diffuseColour;
    float    Kd;
    float    Ks;
    float    shininess;
    BOOL     hasDiffuseTex;
};

struct PBRPhongMaterialData
{
    XMFLOAT3 diffuseColour;
    BOOL     hasDiffuseTex;

    XMFLOAT3 specularColour;
    float    shininess;
};

struct MetallicRoughnessMaterialData
{
    XMFLOAT4 baseColour;
    float    metallicFactor;
    float    roughnessFactor;
    float    occlusionStrength;
    float    normalScale;
    BOOL     hasBaseColourTex;
    BOOL     hasMetallicRoughnessTex;
    BOOL     hasOcclusionTex;
    BOOL     hasNormalMap;
    BOOL     hasEmissive;
};

class BasicMaterial
{
public:
    enum Type
    {
        BASIC = 0,
        PHONG,   
        PBR_PHONG,
        METALLIC_ROUGHNESS
    };

public:

    BasicMaterial();
    ~BasicMaterial();

    void load(const tinygltf::Model& model, const tinygltf::Material& material, Type materialType, const char* basePath);

    const ShaderTableDesc& getTexturesTableDesc() const { return textureTableDesc;}

    Type getMaterialType() const { return materialType;  }

    const BasicMaterialData& getBasicMaterial() const { _ASSERTE(materialType == BASIC); return materialData.basic; }
    const PhongMaterialData& getPhongMaterial() const { _ASSERTE(materialType == PHONG); return materialData.phong; }
    const PBRPhongMaterialData& getPBRPhongMaterial() const { _ASSERTE(materialType == PBR_PHONG); return materialData.pbrPhong; }
    const MetallicRoughnessMaterialData& getMetallicRoughnessMaterial() const { _ASSERTE(materialType == METALLIC_ROUGHNESS); return materialData.metallicRoughness; }

    void setPhongMaterial(const PhongMaterialData& phong);
    void setPBRPhongMaterial(const PBRPhongMaterialData& pbr);
    void setMetallicRoughnessMaterial(const MetallicRoughnessMaterialData& pbr);

    const char* getName() const { return name.c_str(); }

private:

    union 
    {
        BasicMaterialData             basic;
        PhongMaterialData             phong;
        PBRPhongMaterialData          pbrPhong;
        MetallicRoughnessMaterialData metallicRoughness;
    }                        materialData;
    Type                     materialType = BASIC;

    ComPtr<ID3D12Resource>  baseColourTex;
    ComPtr<ID3D12Resource>  metallicRoughnessTex;
    ComPtr<ID3D12Resource>  occlusionTex;
    ComPtr<ID3D12Resource>  normalTex;
    ComPtr<ID3D12Resource>  emissiveTex;
    ShaderTableDesc         textureTableDesc;
    std::string             name;
};
