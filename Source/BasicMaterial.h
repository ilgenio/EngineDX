#pragma once

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

class BasicMaterial
{
public:
    enum Type
    {
        BASIC = 0,
        PHONG,        
    };

public:

    BasicMaterial();
    ~BasicMaterial();

    void load(const tinygltf::Model& model, const tinygltf::Material& material, Type materialType, const char* basePath);

    UINT getTexturesTableDescriptor() const { return baseColourSRV;}
    Type getMaterialType() const { return materialType;  }

    const BasicMaterialData& getBasicMaterial() const { _ASSERTE(materialType == BASIC); return materialData.basic; }
    const PhongMaterialData& getPhongMaterial() const { _ASSERTE(materialType == PHONG); return materialData.phong; }

    const char* getName() const { return name.c_str(); }

private:

    union 
    {
        BasicMaterialData basic;
        PhongMaterialData phong;
    }                       materialData;
    Type                    materialType = BASIC;

    ComPtr<ID3D12Resource>  baseColourTex;
    UINT                    baseColourSRV = UINT32_MAX;
    std::string             name;
};
