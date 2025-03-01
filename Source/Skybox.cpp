#include "Globals.h"

#include "Skybox.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleDescriptors.h"

namespace
{
    enum DescritporSlots
    {
        BACKGOURND_DESC_SLOT = 0,
        DIFFUSE_DESC_SLOT,
        SPECULAR_DESC_SLOT,
        BRDF_DESC_SLOT,
        NUM_DESC_SLOTS
    };
}

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

    ModuleDescriptors* descriptors  = app->getDescriptors();
    descriptors->allocateDescGroup(NUM_DESC_SLOTS, descGroup);

    descriptors->createTextureSRV(background.Get(), descGroup, BACKGOURND_DESC_SLOT);
    descriptors->createTextureSRV(diffuse.Get(), descGroup, DIFFUSE_DESC_SLOT);
    descriptors->createTextureSRV(specular.Get(), descGroup, SPECULAR_DESC_SLOT);
    descriptors->createTextureSRV(brdf.Get(), descGroup, BRDF_DESC_SLOT);
}

