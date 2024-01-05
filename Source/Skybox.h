#pragma once

#include "CubemapMesh.h"
#include "DescriptorHeaps.h"


class Skybox
{
public:

    Skybox();
    ~Skybox();

    void load(const char* backgroundFile, const char* specularFile, const char* diffuseFile, const char* brdfFile);

    const CubemapMesh& getCubemapMesh() const { return mesh;  }

    uint32_t getSpecularIBLMipLevels() const {return iblMipLevels; }

    ID3D12Resource* getBackgroundImage() const {return background.Get(); }
    ID3D12Resource* getDiffuseImage() const {return diffuse.Get();}
    ID3D12Resource* getSpecularImage() const {return specular.Get();}
    ID3D12Resource* getBRDFImage() const {return brdf.Get();}

    const DescriptorGroup getDescriptors() const { return descGroup; }

private:
    ComPtr<ID3D12Resource>   background;

    ComPtr<ID3D12Resource>  specular;
    ComPtr<ID3D12Resource>  diffuse;
    ComPtr<ID3D12Resource>  brdf;
    DescriptorGroup         descGroup;

    CubemapMesh             mesh;

    uint32_t                iblMipLevels = 0;
};
