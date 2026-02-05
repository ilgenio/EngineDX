#include "Globals.h"

#include "GBuffer.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleTargetDescriptors.h"

const DXGI_FORMAT GBuffer::gBufferFormats[BUFFER_COUNT] =
{
    DXGI_FORMAT_R8G8B8A8_UNORM,     // Albedo+alpha
    DXGI_FORMAT_R32G32B32A32_FLOAT, // Normal+MetallicRoughness
    DXGI_FORMAT_R32G32B32A32_FLOAT  // Emissive+AO
};

const char* GBuffer::gBufferNames[BUFFER_COUNT] =
{
    "GBuffer_Albedo",
    "GBuffer_Normal_MetallicRoughness",
    "GBuffer_Emissive_AO"
};

const DXGI_FORMAT GBuffer::depthFormat = DXGI_FORMAT_D32_FLOAT;

GBuffer::GBuffer()
{
}

GBuffer::~GBuffer()
{
}

void GBuffer::resize(UINT width, UINT height)
{
    if(this->width == width && this->height == height)
        return;


    this->width = width;
    this->height = height;

    // Resize logic for GBuffer textures
    ModuleResources* resources = app->getResources();
    ModuleTargetDescriptors* targetDescriptors = app->getTargetDescriptors();
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();

    srvDesc = descriptors->allocTable();

    Vector4 clearColour = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

    // Create Render Target 
    for(int i = 0; i < BUFFER_COUNT; ++i)
    {
        resources->deferRelease(textures[i]);
        textures[i] = resources->createRenderTarget( gBufferFormats[i], size_t(width), size_t(height), 1, clearColour, gBufferNames[i]);
        rtvDesc[i] = targetDescriptors->createRT(textures[i].Get());
        srvDesc.createTextureSRV(textures[i].Get(), i);
    }

    // Create Depth Texture
    resources->deferRelease(depthTexture);
    depthTexture = resources->createDepthStencil(depthFormat, size_t(width), size_t(height), 1, 1.0, 0, "GBuffer_Depth");

    // Create DSV
    dsvDesc = targetDescriptors->createDS(depthTexture.Get());
}

void GBuffer::transitionToRTV(ID3D12GraphicsCommandList* cmdList)
{
    CD3DX12_RESOURCE_BARRIER toRT[BUFFER_COUNT];
    for(int i = 0; i < BUFFER_COUNT; ++i)
    {
        toRT[i] = CD3DX12_RESOURCE_BARRIER::Transition(textures[i].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    cmdList->ResourceBarrier(BUFFER_COUNT, &toRT[0]);
}


void GBuffer::transitionToSRV(ID3D12GraphicsCommandList* cmdList)
{
    CD3DX12_RESOURCE_BARRIER toSRV[BUFFER_COUNT];
    for(int i = 0; i < BUFFER_COUNT; ++i)
    {
        toSRV[i] = CD3DX12_RESOURCE_BARRIER::Transition(textures[i].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
    cmdList->ResourceBarrier(BUFFER_COUNT, &toSRV[0]);
}

void GBuffer::setRenderTarget(ID3D12GraphicsCommandList* cmdList)
{
    // Implementation for setting GBuffer textures as render targets
    ModuleD3D12* d3d12 = app->getD3D12();

    D3D12_CPU_DESCRIPTOR_HANDLE rtv[GBuffer::BUFFER_COUNT]; 

    for(int i = 0; i < BUFFER_COUNT; ++i)
    {
        rtv[i] = rtvDesc[i].getCPUHandle();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsvDesc.getCPUHandle();
    cmdList->OMSetRenderTargets(BUFFER_COUNT, &rtv[0], FALSE, &dsv);

    Vector4 clearColour = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

    for(int i = 0; i < BUFFER_COUNT; ++i)
    {
        cmdList->ClearRenderTargetView(rtvDesc[i].getCPUHandle(), reinterpret_cast<float*>(&clearColour), 0, nullptr);
    }

    cmdList->ClearDepthStencilView(dsvDesc.getCPUHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, LONG(width), LONG(height) };
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);
}
