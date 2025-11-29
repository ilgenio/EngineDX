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
        FLAG_HAS_NORMAL_TEX            = 0x4,
        FLAG_HAS_COMPRESSED_NORMAL     = 0x8,
        FLAG_HAS_OCCLUSION_TEX         = 0x10,
        FLAG_HAS_EMISSIVE_TEX          = 0x20,
    };

    struct Data
    {
        Vector4     baseColor = Vector4::One;
        float       metallicFactor = 1.0f;
        float       roughnessFactor = 1.0f;
        float       normalScale = 1.0f;
        float       occlusionStrength = 1.0f;
        Vector3     emissiveFactor = Vector3::Zero;
        float       alphaCutoff = 0.5f;
        UINT        flags = 0;                   
        UINT        padding[2] = { 0, 0 };
   };

private:

    enum     {
        TEX_SLOT_BASECOLOUR = 0,
        TEX_SLOT_METALLIC_ROUGHNESS = 1,
        TEX_SLOT_NORMAL = 2,
        TEX_SLOT_OCCLUSION = 3,
        TEX_SLOT_EMISSIVE = 4,
        TEX_SLOT_COUNT
    };

    struct TextureInfo
    {
        std::filesystem::path path;
        ComPtr<ID3D12Resource> texture;
    };

    std::string             name;
    Data                    data;
    TextureInfo             texutures[TEX_SLOT_COUNT];
    ALPHAMODE               alphaMode = ALPHA_MODE_OPAQUE;
    ShaderTableDesc         textureTableDesc;

public:

    Material();
    ~Material();

    void load(const tinygltf::Model& model, const tinygltf::Material& material, const char* basePath);

    const std::string_view& getName() const { return name; }
    ALPHAMODE getAlphaMode() const {return alphaMode;}
    const Data& getData() const { return data; }

    D3D12_GPU_DESCRIPTOR_HANDLE getTextureTable() const { return textureTableDesc.getGPUHandle(); }
    static UINT getNumTextureSlots() { return TEX_SLOT_COUNT; }

private:

    bool loadTexture(const tinygltf::Model& model, const std::string& basePath, int index, bool defaultSRGB, TextureInfo& output);

};
