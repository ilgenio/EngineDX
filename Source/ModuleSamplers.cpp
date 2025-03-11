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
    ModuleD3D12* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = COUNT;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap));

    cpuStart = heap->GetCPUDescriptorHandleForHeapStart();
    gpuStart = heap->GetGPUDescriptorHandleForHeapStart();

    D3D12_SAMPLER_DESC samplers[COUNT] = {
        {
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            0.0f, 16, D3D12_COMPARISON_FUNC_LESS_EQUAL,
            {0.0f, 0.0f, 0.0f, 0.0f},
            0.0f, D3D12_FLOAT32_MAX
        },

        {
            D3D12_FILTER_MIN_MAG_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            0, 16, D3D12_COMPARISON_FUNC_LESS_EQUAL,
            {0.0f, 0.0f, 0.0f, 0.0f},
            0.0f, D3D12_FLOAT32_MAX
        },

        {
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            0, 16, D3D12_COMPARISON_FUNC_LESS_EQUAL,
            {0.0f, 0.0f, 0.0f, 0.0f},
            0.0f, D3D12_FLOAT32_MAX
        },

        {
            D3D12_FILTER_MIN_MAG_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            0, 16, D3D12_COMPARISON_FUNC_LESS_EQUAL,
            {0.0f, 0.0f, 0.0f, 0.0f},
            0.0f, D3D12_FLOAT32_MAX
        }
    };

    for (uint32_t i = 0; i < COUNT; ++i)
    {
        device->CreateSampler(&samplers[i], CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, i, descriptorSize));
    }
  
    return true;
}

bool ModuleSamplers::cleanUp() 
{
    return true;
}
