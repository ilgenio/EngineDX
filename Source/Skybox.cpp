#include "Globals.h"

#include "Skybox.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "SingleDescriptors.h"

void Skybox::load(const char *backgroundFile, const char* diffuseFile, const char* specularFile, const char* brdfFile)
{
    ModuleResources* resources = app->getResources();

    auto loadTexture = [=](const char* fileName, ComPtr<ID3D12Resource>& texture)
    {
        texture  = resources->createTextureFromFile(backgroundFile);
    };

    loadTexture(backgroundFile, background);
    loadTexture(diffuseFile, diffuse);
    loadTexture(specularFile, specular);
    loadTexture(brdfFile, brdf);

    if (specular)
    {
        iblMipLevels = specular->GetDesc().MipLevels;
    }

    SingleDescriptors* descriptors  = app->getShaderDescriptors()->getSingle();

    backgroundDesc = descriptors->createTextureSRV(background.Get());
    diffuseDesc = descriptors->createTextureSRV(diffuse.Get());
    specularDesc = descriptors->createTextureSRV(specular.Get());
    brdfDesc  = descriptors->createTextureSRV(brdf.Get());
}

