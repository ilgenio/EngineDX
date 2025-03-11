#pragma once

#include "DescriptorHeaps.h"

namespace tinygltf { struct Material; class Model; }

class Material
{
public:

    enum ALPHAMODE
    {
        ALPHA_MODE_OPAQUE,
        ALPHA_MODE_MASK,
        ALPHA_MODE_BLEND
    };

public:

    Material();
    ~Material();

    void load(const tinygltf::Model& model, const tinygltf::Material& material, const char* basePath);

    const Vector4& getBaseColor() const {return data.baseColor; }
    const ID3D12Resource* getBaseColorTex() const { return baseColorTex.Get(); }
    const ID3D12Resource* getMetRougTex() const { return metRougTex.Get(); }
    const ID3D12Resource* getNormalTex() const { return normalTex.Get(); }
    float getMetallicFactor() const {return data.metallicFactor; }
    float getRoughnessFactor() const {return data.roughnessFactor; }

    ALPHAMODE getAlphaMode() const {return alphaMode;}

    static ID3D12RootSignature* getRootSignature() { return rootSignature.Get(); }

    static void createSharedData();
    static void destroySharedData();

private:

    void loadIridescenceExt(const tinygltf::Model& model, const tinygltf::Material &material, const std::string& basePath);
    void loadTransmissionExt(const tinygltf::Model& model, const tinygltf::Material &material, const std::string& basePath);
    void loadIORExt(const tinygltf::Material &material);
    bool loadTexture(const tinygltf::Model& model, const std::string& basePath, int index, ComPtr<ID3D12Resource>& output);

private:
    struct Iridescence
    {        
        float factor = 0.0f;
        float IOR = 0.0f;
        float maxThikness = 0.0f;
        float minThikness = 0.0f;
    };

    enum FLAGS
    {
        MATERIAL_FLAGS_IRIDESCENCE = 1 << 0,
        MATERIAL_FLAGS_IRIDESCENCE_TEX = 1 << 1, 
        MATERIAL_FLAGS_IRIDESCENCE_THICKNESS_TEX = 1 << 2,
        MATERIAL_FLAGS_TRANSMISSION = 1 << 3,
        MATERIAL_FLAGS_TRANSMISSION_TEX = 1 << 4
    };

    struct MaterialData
    {
        Vector4      baseColor = Vector4::One;
        float        metallicFactor = 1.0f;
        float        roughnessFactor = 1.0f;
        float        normalScale = 1.0f;
        int          flags = 0;
        Iridescence  iridescence;
        float        dielectricF0 = 0.04f;
        float        transmissionFactor = 0.0f;
        int          padding0;
        int          padding1;
        int          padding2;
    };

    std::string             name;
    MaterialData            data;
    ComPtr<ID3D12Resource>  materialBuffer;
    ComPtr<ID3D12Resource>  baseColorTex;
    ComPtr<ID3D12Resource>  metRougTex;
    ComPtr<ID3D12Resource>  normalTex;
    ComPtr<ID3D12Resource>  occlusionTex;
    ComPtr<ID3D12Resource>  iridescenceTex;
    ComPtr<ID3D12Resource>  iridescenceThicknessTex;
    ComPtr<ID3D12Resource>  transmissionTex;
    ALPHAMODE               alphaMode = ALPHA_MODE_OPAQUE;
    float                   alphaCutoff = 0.5f;
    UINT                    baseColourDesc = UINT32_MAX;
    UINT                    metalRoughDesc = UINT32_MAX;
    UINT                    normalDesc = UINT32_MAX;
    UINT                    occlusionDesc = UINT32_MAX;
    UINT                    iridiscenceDesc = UINT32_MAX;
    UINT                    iridiscenceThicknessDesc = UINT32_MAX;
    UINT                    transmissionDesc = UINT32_MAX;
    UINT                    materialDesc = UINT32_MAX;

    static ComPtr<ID3D12RootSignature> rootSignature;
    static ComPtr<ID3D12Resource> whiteFallback;
    static ComPtr<ID3D12Resource> blackFallback;
    static ComPtr<ID3D12Resource> normalFallback;
};
