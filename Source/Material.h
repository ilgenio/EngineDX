#pragma once


#include "ShaderTableDesc.h"

namespace tinygltf { struct Material; class Model; }

class Material
{
public:

    enum ALPHAMODE
    {
        ALPHA_MODE_OPAQUE = 0,
        ALPHA_MODE_BLEND  = 2,
    };

    enum FLAGS
    {
        FLAG_HAS_BASECOLOUR_TEX        = 0x1,
        FLAG_HAS_METALLICROUGHNESS_TEX = 0x2,
    };

    struct Data
    {
        Vector4     baseColor = Vector4::One;
        float       metallicFactor = 1.0f;
        float       roughnessFactor = 1.0f;
        float       normalScale = 1.0f;
        float       alphaCutoff = 0.5f;
        float       occlusionStrength = 1.0f;
        UINT        flags = 0;                   
        UINT        padding[2] = { 0, 0 };
   };

public:

    Material();
    ~Material();

    void load(const tinygltf::Model& model, const tinygltf::Material& material, const char* basePath);

    const std::string_view& getName() const { return name; }
    ALPHAMODE getAlphaMode() const {return alphaMode;}
    const Data& getData() const { return data; }

    D3D12_GPU_DESCRIPTOR_HANDLE getTextureTable() const { return textureTableDesc.getGPUHandle(); }

private:

    bool loadTexture(const tinygltf::Model& model, const std::string& basePath, int index, bool defaultSRGB, ComPtr<ID3D12Resource>& output);

private:

    enum     {
        TEX_SLOT_BASECOLOUR = 0,
        TEX_SLOT_METALLIC_ROUGHNESS = 1,
        TEX_SLOT_NORMAL = 2,
        TEX_SLOT_OCCLUSION = 3,
        TEX_SLOT_COUNT = 4
    };

    std::string             name;
    Data                    data;
    ComPtr<ID3D12Resource>  baseColorTex;
    ComPtr<ID3D12Resource>  metRougTex;
    ComPtr<ID3D12Resource>  normalTex;
    ComPtr<ID3D12Resource>  occlusionTex;
    ALPHAMODE               alphaMode = ALPHA_MODE_OPAQUE;
    ShaderTableDesc         textureTableDesc;
};
