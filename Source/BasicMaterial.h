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

struct PBRPhongMaterialData
{
    XMFLOAT3 diffuseColour;
    BOOL     hasDiffuseTex;

    XMFLOAT3 specularColour;
    float    shininess;
};

class BasicMaterial
{
public:
    enum Type
    {
        BASIC = 0,
        PHONG,   
        PBR_PHONG
    };

public:

    BasicMaterial();
    ~BasicMaterial();

    void load(const tinygltf::Model& model, const tinygltf::Material& material, Type materialType, const char* basePath);

    UINT getTexturesTableDescriptor() const { return baseColourSRV;}
    Type getMaterialType() const { return materialType;  }

    const BasicMaterialData& getBasicMaterial() const { _ASSERTE(materialType == BASIC); return materialData.basic; }
    const PhongMaterialData& getPhongMaterial() const { _ASSERTE(materialType == PHONG); return materialData.phong; }
    const PBRPhongMaterialData& getPBRPhongMaterial() const { _ASSERTE(materialType == PBR_PHONG); return materialData.pbrPhong; }

    void setPhongMaterial(const PhongMaterialData& phong);
    void setPBRPhongMaterial(const PBRPhongMaterialData& pbr);

    const char* getName() const { return name.c_str(); }

private:

    union 
    {
        BasicMaterialData    basic;
        PhongMaterialData    phong;
        PBRPhongMaterialData pbrPhong;
    }                        materialData;
    Type                     materialType = BASIC;

    ComPtr<ID3D12Resource>  baseColourTex;
    UINT                    baseColourSRV = 0;
    std::string             name;
};
