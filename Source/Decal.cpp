#include "Globals.h"

#include "Decal.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"

Decal::Decal(const char* colorPath, const char* normalPath, const char* aoPath, const Matrix& transform)
{
    // TODO: Add to scene for sharing textures and do decal frustum culling

    ModuleResources* resources = app->getResources();
    ModuleShaderDescriptors* shaderDescriptors = app->getShaderDescriptors();

    textureTableDesc = shaderDescriptors->allocTable();

    if(colorPath)
    {
        color = resources->createTextureFromFile(colorPath, true);
        materialFlags |= FLAG_HAS_COLOR;

        textureTableDesc.createTextureSRV(color.Get(), TEX_SLOT_BASECOLOUR);
    }

    if(normalPath)
    {
        normal = resources->createTextureFromFile(normalPath, false);
        materialFlags |= FLAG_HAS_NORMAL;
        textureTableDesc.createTextureSRV(normal.Get(), TEX_SLOT_NORMAL);
    }

    if(aoPath)
    {
        ambientOcclusion = resources->createTextureFromFile(aoPath, false);
        materialFlags |= FLAG_HAS_AO;
        textureTableDesc.createTextureSRV(ambientOcclusion.Get(), TEX_SLOT_OCCLUSION);
    }

    this->transform = transform;
}

Decal::Decal()
{

}

Decal::~Decal()
{

}

