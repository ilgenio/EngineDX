#include "Globals.h"

#include "Skybox.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleVulkan.h"

Skybox::Skybox()
{
}

Skybox::~Skybox()
{
    clean();
}

void Skybox::load(const char *backgroundFile, const char* diffuseFile, const char* specularFile, const char* brdfFile)
{
    clean();
    ModuleResources* resources = App->getResources();
    ModuleVulkan* vulkan = App->getVulkan();

    auto loadTexture = [=](const char* fileName, Texture& texture, const ImageMetadata** metadata, bool toCubemap, uint32_t cubemapSize)
    {
        texture.path = resources->normalizeUniquePath(path(fileName));
        resources->createUniqueImage(texture.path, texture.image, texture.allocation, texture.view, metadata, toCubemap, cubemapSize);
    };

    const ImageMetadata* specularMeta = nullptr;

    loadTexture(backgroundFile, background, nullptr, true, 1024);
    loadTexture(diffuseFile, diffuse, nullptr, false, 0);
    loadTexture(specularFile, specular, &specularMeta, false, 0);
    loadTexture(brdfFile, brdf, nullptr, false, 0);

    iblMipLevels = specularMeta->mipCount;
}

void Skybox::clean()
{
    ModuleResources* resources = App->getResources();
    if (!background.path.empty()) resources->destroyUniqueImage(background.path);
    if (!diffuse.path.empty()) resources->destroyUniqueImage(diffuse.path);
    if (!specular.path.empty()) resources->destroyUniqueImage(specular.path);
    if (!brdf.path.empty()) resources->destroyUniqueImage(brdf.path);
}
