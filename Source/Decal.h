#pragma once

#include "ShaderTableDesc.h"

class Decal
{
    enum EMaterialFlags
    {
        FLAG_HAS_COLOR = 1 << 0,
        FLAG_HAS_NORMAL = 1 << 1,
        FLAG_HAS_AO = 1 << 2
    };

    enum {
        TEX_SLOT_BASECOLOUR = 0,
        TEX_SLOT_NORMAL = 1,
        TEX_SLOT_OCCLUSION = 2,
        TEX_SLOT_COUNT
    };


    Matrix transform;

    ComPtr<ID3D12Resource> color;
    ComPtr<ID3D12Resource> normal;
    ComPtr<ID3D12Resource> ambientOcclusion;

    ShaderTableDesc  textureTableDesc;

    UINT materialFlags = 0; 

public:

    Decal(const char* colorPath, const char* normalPath, const char* aoPath, const Matrix& transform);
    Decal();
    ~Decal();

    const Matrix& getTransform() const { return transform; }

    ID3D12Resource* getColor() const { return color.Get(); }
    ID3D12Resource* getNormal() const { return normal.Get(); }
    ID3D12Resource* getAmbientOcclusion() const { return ambientOcclusion.Get(); }

    bool hasColor() const { return (materialFlags & FLAG_HAS_COLOR) != 0; }
    bool hasNormal() const { return (materialFlags & FLAG_HAS_NORMAL) != 0; }
    bool hasAO() const { return (materialFlags & FLAG_HAS_AO) != 0; }   

    const ShaderTableDesc& getTextureTableDesc() const { return textureTableDesc; }
};