#include "Globals.h"

#include "ModuleSamplers.h"
#include "Application.h"
#include "ModuleD3D12.h"


ModuleSamplers::ModuleSamplers()
{
}

ModuleSamplers::~ModuleSamplers()
{
}

bool ModuleSamplers::init() 
{
    heap.init(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 128);
    heap.allocate(4, defaultSamplers);

    D3D12_SAMPLER_DESC wrapLinear =
    {
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        0, 16, D3D12_COMPARISON_FUNC_LESS_EQUAL, 
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE, 
        0.0f, D3D12_FLOAT32_MAX
    };

    D3D12_SAMPLER_DESC wrapPoint =
    {
        D3D12_FILTER_MIN_MAG_MIP_POINT, 
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        0, 16, D3D12_COMPARISON_FUNC_LESS_EQUAL, 
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE, 
        0.0f, D3D12_FLOAT32_MAX
    };

    D3D12_SAMPLER_DESC clampLinear =
    {
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        0, 16, D3D12_COMPARISON_FUNC_LESS_EQUAL, 
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE, 
        0.0f, D3D12_FLOAT32_MAX
    };

    D3D12_SAMPLER_DESC clampPoint =
    {
        D3D12_FILTER_MIN_MAG_MIP_POINT, 
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        0, 16, D3D12_COMPARISON_FUNC_LESS_EQUAL, 
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE, 
        0.0f, D3D12_FLOAT32_MAX
    };

    ID3D12Device5* device = app->getD3D12()->getDevice();

    device->CreateSampler(&wrapLinear, defaultSamplers.getCPU(0));
    device->CreateSampler(&wrapPoint, defaultSamplers.getCPU(1));
    device->CreateSampler(&clampLinear, defaultSamplers.getCPU(2));
    device->CreateSampler(&clampPoint, defaultSamplers.getCPU(3));
  
    return true;
}

bool ModuleSamplers::cleanUp() 
{
    return true;
}
