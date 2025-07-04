#pragma once

#include "CubemapMesh.h"



class Skybox
{
public:

    void load(const char* backgroundFile, const char* specularFile, const char* diffuseFile, const char* brdfFile);

    const CubemapMesh& getCubemapMesh() const { return mesh;  }

    uint32_t getSpecularIBLMipLevels() const {return iblMipLevels; }

    ID3D12Resource* getBackgroundImage() const {return background.Get(); }
    ID3D12Resource* getDiffuseImage() const {return diffuse.Get();}
    ID3D12Resource* getSpecularImage() const {return specular.Get();}
    ID3D12Resource* getBRDFImage() const {return brdf.Get();}

private:
    ComPtr<ID3D12Resource>   background;

    ComPtr<ID3D12Resource>  specular;
    ComPtr<ID3D12Resource>  diffuse;
    ComPtr<ID3D12Resource>  brdf;
    UINT                    backgroundDesc = UINT32_MAX;
    UINT                    specularDesc = UINT32_MAX;
    UINT                    diffuseDesc = UINT32_MAX;
    UINT                    brdfDesc = UINT32_MAX;

    CubemapMesh             mesh;
    uint32_t                iblMipLevels = 0;
};
