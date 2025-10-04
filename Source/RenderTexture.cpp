#include "Globals.h"
#include "RenderTexture.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleRTDescriptors.h"
#include "ModuleDSDescriptors.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleD3D12.h"

RenderTexture::~RenderTexture()
{
}

void RenderTexture::resize(UINT width, UINT height)
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
    rtvDesc = rtDescriptors->create(texture.Get());

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
        dsvDesc = dsDescriptors->create(depthTexture.Get());
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

void RenderTexture::setRenderTarget(ID3D12GraphicsCommandList* cmdList)
{
    ModuleD3D12* d3d12 = app->getD3D12();

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvDesc.getCPUHandle();

    if (depthFormat == DXGI_FORMAT_UNKNOWN)
    {
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        cmdList->ClearRenderTargetView(rtvDesc.getCPUHandle(), reinterpret_cast<float*>(&clearColour), 0, nullptr);
    }
    else
    {
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsvDesc.getCPUHandle();
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
        cmdList->ClearRenderTargetView(rtvDesc.getCPUHandle(), reinterpret_cast<float*>(&clearColour), 0, nullptr);
        cmdList->ClearDepthStencilView(dsvDesc.getCPUHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    }

    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, LONG(width), LONG(height) };
    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);
}

void RenderTexture::bindAsShaderResource(ID3D12GraphicsCommandList *cmdList, int slot)
{
    cmdList->SetGraphicsRootDescriptorTable(slot, getSrvHandle());
}
