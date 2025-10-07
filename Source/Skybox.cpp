#include "Globals.h"

#include "Skybox.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleCamera.h"

#include "IrradianceMapPass.h"
#include "PrefilterEnvMapPass.h"    
#include "EnvironmentBRDFPass.h"
#include "HDRToCubemapPass.h"
#include "SkyboxRenderPass.h"

#define DEFAULT_SKYBOX_SIZE 1024
#define DEFAULT_ENV_BRDF_SIZE 128
#define DEFAULT_MIP_LEVELS 8

#ifdef _DEBUG
#define CAPTURE_IBL_GENERATION 1
#else
#define CAPTURE_IBL_GENERATION 0
#endif 

Skybox::Skybox()
{
    irradianceMapPass   = std::make_unique<IrradianceMapPass>();
    prefilterEnvMapPass = std::make_unique<PrefilterEnvMapPass>();
    environmentBRDFPass = std::make_unique<EnvironmentBRDFPass>();  
    hdrToCubemapPass    = std::make_unique<HDRToCubemapPass>();
    skyboxRenderPass    = std::make_unique<SkyboxRenderPass>();
}

Skybox::~Skybox()
{

}

bool Skybox::loadHDR(const char* hdrFile)
{
    ModuleResources* resources = app->getResources();

    ComPtr<ID3D12Resource> hdr = resources->createTextureFromFile(hdrFile);
    if (!hdr)
    {
        LOG("Error loading HDR image %s", hdrFile);
        return false;
    }

    tableDesc = app->getShaderDescriptors()->allocTable();

    tableDesc.createTextureSRV(hdr.Get(), TEX_SLOT_HDR);

#if CAPTURE_IBL_GENERATION
    if(PIXIsAttachedForGpuCapture()) PIXBeginCapture(PIX_CAPTURE_GPU, nullptr);
#endif

    skybox            = hdrToCubemapPass->generate(tableDesc.getGPUHandle(TEX_SLOT_HDR), DXGI_FORMAT_R16G16B16A16_FLOAT, DEFAULT_SKYBOX_SIZE);
    tableDesc.createCubeTextureSRV(skybox.Get(), TEX_SLOT_SKYBOX);

    irradianceMap     = irradianceMapPass->generate(tableDesc.getGPUHandle(TEX_SLOT_SKYBOX), DEFAULT_SKYBOX_SIZE, DEFAULT_SKYBOX_SIZE);

    iblMipLevels      = DEFAULT_MIP_LEVELS;
    prefilteredEnvMap = prefilterEnvMapPass->generate(tableDesc.getGPUHandle(TEX_SLOT_SKYBOX), DEFAULT_SKYBOX_SIZE, DEFAULT_SKYBOX_SIZE, iblMipLevels);

    if(!environmentBRDF) environmentBRDF   = environmentBRDFPass->generate(DEFAULT_ENV_BRDF_SIZE);

#if CAPTURE_IBL_GENERATION
    if (PIXIsAttachedForGpuCapture()) PIXEndCapture(TRUE);
#endif

    tableDesc.createCubeTextureSRV(irradianceMap.Get(), TEX_SLOT_IRRADIANCE);
    tableDesc.createCubeTextureSRV(prefilteredEnvMap.Get(), TEX_SLOT_PREFILTERED_ENV);
    tableDesc.createTextureSRV(environmentBRDF.Get(), TEX_SLOT_ENV_BRDF);

    return true;
}

void Skybox::render(ID3D12GraphicsCommandList *cmdList, float aspectRatio)
{
    skyboxRenderPass->record(cmdList, tableDesc.getGPUHandle(TEX_SLOT_SKYBOX), app->getCamera()->getRot(), ModuleCamera::getPerspectiveProj(aspectRatio));
}
    
