#pragma once

namespace tinygltf { class Model; struct Material;  }

class BasicMaterial
{
public:

    BasicMaterial();
    ~BasicMaterial();

    void load(const tinygltf::Model& model, const tinygltf::Material& material, const char* basePath);

    UINT getTableStartDescriptor() const { return baseColourSRV;  }

private:

    struct MaterialData
    {
        Vector4 baseColour       = Vector4::One;
        BOOL    hasColourTexture = 0;
    };

    MaterialData            materialData;
    ComPtr<ID3D12Resource>  baseColourTex;
    UINT                    baseColourSRV = UINT32_MAX;
    ComPtr<ID3D12Resource>  materialBuffer;
    UINT                    materialCBV = UINT32_MAX;
    std::string             name;
};
