#include "Globals.h"
#include "RenderTexture.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleRTDescriptors.h"
#include "ModuleDSDescriptors.h"
#include "ModuleShaderDescriptors.h"
#include "SingleDescriptors.h"
#include "ModuleD3D12.h"

RenderTexture::~RenderTexture()
{
    if (texture)
    {
        app->getRTDescriptors()->release(rtvHandle);

        if (depthFormat != DXGI_FORMAT_UNKNOWN)
        {
            ModuleDSDescriptors *dsDescriptors = app->getDSDescriptors();
            dsDescriptors->release(dsvHandle);
        }
    }
}

void RenderTexture::resize(int width, int height)
{
    if(this->width == width && this->height == height)
        return;

    this->width = width;
    this->height = height;

    ModuleResources* resources = app->getResources();
    ModuleRTDescriptors* rtDescriptors = app->getRTDescriptors();
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();

    // Create Render Target 
    resources->deferRelease(texture);
    texture = resources->createRenderTarget(format, size_t(width), size_t(height), clearColour, name);

    // Create RTV.
    rtDescriptors->release(rtvHandle);
    rtvHandle = rtDescriptors->create(texture.Get());

    // Create SRV.
    srvDesc = descriptors->allocTable();
    srvDesc.createTextureSRV(texture.Get());

    if(depthFormat != DXGI_FORMAT_UNKNOWN)
    {
        ModuleDSDescriptors *dsDescriptors = app->getDSDescriptors();

        // Create Depth Texture
        resources->deferRelease(depthTexture);
        depthTexture = resources->createDepthStencil(depthFormat, size_t(width), size_t(height), clearDepth, 0, name);

        // Create DSV
        dsDescriptors->release(dsvHandle);
        dsvHandle = dsDescriptors->create(depthTexture.Get());
    }
}

void RenderTexture::transitionToRTV(ID3D12GraphicsCommandList* cmdList)
{
    CD3DX12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmdList->ResourceBarrier(1, &toRT);
}

void RenderTexture::transitionToSRV(ID3D12GraphicsCommandList* cmdList)
{
    CD3DX12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &toSRV);
}

void RenderTexture::bindAsRenderTarget(ID3D12GraphicsCommandList *cmdList)
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = app->getRTDescriptors()->getCPUHandle(rtvHandle);

    if(depthFormat == DXGI_FORMAT_UNKNOWN)
    {
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    }
    else
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = app->getDSDescriptors()->getCPUHandle(dsvHandle);
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    }
}

void RenderTexture::clear(ID3D12GraphicsCommandList* cmdList)
{
    cmdList->ClearRenderTargetView(app->getRTDescriptors()->getCPUHandle(rtvHandle), reinterpret_cast<float*>(&clearColour), 0, nullptr);

    if (depthFormat != DXGI_FORMAT_UNKNOWN)
    {
        cmdList->ClearDepthStencilView(app->getDSDescriptors()->getCPUHandle(dsvHandle), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    }
}


void RenderTexture::bindAsShaderResource(ID3D12GraphicsCommandList *cmdList, int slot)
{
    cmdList->SetGraphicsRootDescriptorTable(slot, getSRVHandle());
}
