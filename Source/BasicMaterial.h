#pragma once

namespace tinygltf { class Model; struct Material;  }

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

    UINT getTableStartDescriptor() const { return baseColourSRV;  }

private:

    struct BasicData
    {
        XMFLOAT4 baseColour;
        BOOL     hasColourTexture;
    };

    struct PhongData
    {
        XMFLOAT4 diffuseColour;
        float    Kd;
        float    Ks;
        float    shininess;
        BOOL     hasDiffuseTex;
    };

    union 
    {
        BasicData basic;
        PhongData phong;
    }                       materialData;
    Type                    materialType;

    ComPtr<ID3D12Resource>  baseColourTex;
    UINT                    baseColourSRV = UINT32_MAX;
    ComPtr<ID3D12Resource>  materialBuffer;
    UINT                    materialCBV = UINT32_MAX;
    std::string             name;
};
