#pragma once

//#include "CubemapMesh.h"
#include <filesystem>

class Skybox
{
    struct Texture
    {
        std::filesystem::path path;
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };

    typedef std::filesystem::path path;

    Texture                 background;
    
    Texture                 specular;
    Texture                 diffuse;
    Texture                 brdf;

    CubemapMesh             mesh;

    uint32_t                iblMipLevels = 0;

public:

    Skybox();
    ~Skybox();

    void load(const char* backgroundFile, const char* specularFile, const char* diffuseFile, const char* brdfFile);

    const CubemapMesh& getCubemapMesh() const { return mesh;  }

    uint32_t getSpecularIBLMipLevels() const {return iblMipLevels; }

    VkImage getBackgroundImage() const {return background.image;}
    VkImage getDiffuseImage() const {return diffuse.image;}
    VkImage getSpecularImage() const {return specular.image;}
    VkImage getBRDFImage() const {return brdf.image;}

    VkImageView getBackgroundView() const {return background.view;}
    VkImageView getDiffuseView() const {return diffuse.view;}
    VkImageView getSpecularView() const {return specular.view;}
    VkImageView getBRDFView() const {return brdf.view;}

private:
    void clean();
};
