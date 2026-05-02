#pragma once

#include "ShaderTableDesc.h"

class Decal
{
    enum EMaterialFlags
    {
        FLAG_HAS_COLOR = 1 << 0,
        FLAG_HAS_NORMAL = 1 << 1,
    };

    enum {
        TEX_SLOT_BASECOLOUR = 0,
        TEX_SLOT_NORMAL = 1,
        TEX_SLOT_COUNT
    };


    Matrix transform;

    ComPtr<ID3D12Resource> color;
    ComPtr<ID3D12Resource> normal;

    ShaderTableDesc  textureTableDesc;

    UINT materialFlags = 0; 

    std::string colorPath;
    std::string normalPath;

public:

    Decal(const char* colorPath, const char* normalPath, const Matrix& transform);
    Decal();
    ~Decal();

    const Matrix& getTransform() const { return transform; }
    void setTransform(const Matrix& newTransform) { transform = newTransform; }

    ID3D12Resource* getColor() const { return color.Get(); }
    ID3D12Resource* getNormal() const { return normal.Get(); }

    const char* getColorPath() const { return colorPath.c_str(); }
    const char* getNormalPath() const { return normalPath.c_str(); }    

    bool hasColor() const { return (materialFlags & FLAG_HAS_COLOR) != 0; }
    bool hasNormal() const { return (materialFlags & FLAG_HAS_NORMAL) != 0; }

    const ShaderTableDesc& getTextureTableDesc() const { return textureTableDesc; }
};