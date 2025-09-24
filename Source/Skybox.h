#pragma once

#include "CubemapMesh.h"
#include "ShaderTableDesc.h"


class Skybox
{
public:

    void load(const char* backgroundFile, const char* specularFile, const char* diffuseFile, const char* brdfFile);

    const CubemapMesh& getCubemapMesh() const { return mesh;  }

    uint32_t getSpecularIBLMipLevels() const {return iblMipLevels; }
    const ShaderTableDesc& getTextureTableDesc() const { return tableDesc; }

private:
    ComPtr<ID3D12Resource>   background;

    enum 
    {
        TEX_SLOT_BACGROUND = 0,
        TEX_SLOT_SPECULAR,
        TEX_SLOT_DIFFUSE,
        TEX_SLOT_BRDF,
        TEX_SLOT_COUNT
    };

    ComPtr<ID3D12Resource>  specular;
    ComPtr<ID3D12Resource>  diffuse;
    ComPtr<ID3D12Resource>  brdf;
    ShaderTableDesc         tableDesc;

    CubemapMesh             mesh;
    uint32_t                iblMipLevels = 0;
};
