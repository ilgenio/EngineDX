#pragma once

#include "ShaderTableDesc.h"

class IrradianceMapPass;
class PrefilterEnvMapPass;
class EnvironmentBRDFPass;
class HDRToCubemapPass;

// Skybox manages environment lighting resources for physically based rendering (PBR) in a DirectX 12 application.
// It handles loading HDR images, generating cubemaps, irradiance maps, prefiltered environment maps, and BRDF lookup textures.
// Use this class to set up image-based lighting (IBL) for realistic sky and reflections in your scene.
class Skybox
{
    enum 
    {
        TEX_SLOT_IRRADIANCE = 0,
        TEX_SLOT_PREFILTERED_ENV,
        TEX_SLOT_ENV_BRDF,
        TEX_SLOT_SKYBOX,
        TEX_SLOT_HDR,
        TEX_SLOT_COUNT
    };

    std::unique_ptr<IrradianceMapPass>      irradianceMapPass;
    std::unique_ptr<PrefilterEnvMapPass>    prefilterEnvMapPass;
    std::unique_ptr<EnvironmentBRDFPass>    environmentBRDFPass ;
    std::unique_ptr<HDRToCubemapPass>       hdrToCubemapPass;

    ComPtr<ID3D12Resource>  skybox;
    ComPtr<ID3D12Resource>  irradianceMap;
    ComPtr<ID3D12Resource>  prefilteredEnvMap;
    ComPtr<ID3D12Resource>  environmentBRDF;

    ShaderTableDesc         tableDesc;
    UINT                    iblMipLevels = 0;

public:

    Skybox();
    ~Skybox();

    void loadHDR(const char* hdrFileName);

    UINT  getNumIBLMipLevels() const {return iblMipLevels; }
    const ShaderTableDesc& getIBLTableDesc() const { return tableDesc; }
};
