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

    tableDesc = app->getShaderDescriptors()->allocTable();

    tableDesc.createTextureSRV(background.Get(), TEX_SLOT_BACGROUND);
    tableDesc.createTextureSRV(diffuse.Get(), TEX_SLOT_DIFFUSE);
    tableDesc.createTextureSRV(specular.Get(), TEX_SLOT_SPECULAR);
    tableDesc.createTextureSRV(brdf.Get(), TEX_SLOT_BRDF);
}

