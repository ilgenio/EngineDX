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

void RenderTexture::resize(int width, int height)
{
    if(this->width == width && this->height == height)
        return;

    if (texture)
    {
        // Ensure previous texture usage is finished
        app->getD3D12()->flush();
    }

    this->width = width;
    this->height = height;

    ModuleResources* resources = app->getResources();
    ModuleRTDescriptors* rtDescriptors = app->getRTDescriptors();
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    texture = resources->createRenderTarget(format, size_t(width), size_t(height), clearColor, name);

    // Create RTV.
    rtDescriptors->release(rtvHandle);
    rtvHandle = rtDescriptors->create(texture.Get());

    // Create SRV.
    descriptors->release(srvHandle);
    srvHandle = descriptors->createTextureSRV(texture.Get());

    if(depthFormat != DXGI_FORMAT_UNKNOWN)
    {
        ModuleDSDescriptors *dsDescriptors = app->getDSDescriptors();

        depthTexture = resources->createDepthStencil(depthFormat, size_t(width), size_t(height), 1.0f, 0, name);
        // Create DSV
        dsDescriptors->release(dsvHandle);
        dsvHandle = dsDescriptors->create(depthTexture.Get());
    }

}

void RenderTexture::transitionToRTV(ID3D12GraphicsCommandList* commandList)
{
    CD3DX12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &toRT);
}

void RenderTexture::transitionToSRV(ID3D12GraphicsCommandList* commandList)
{
    CD3DX12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &toSRV);
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

void RenderTexture::bindAsShaderResource(ID3D12GraphicsCommandList *cmdList, int slot)
{
    D3D12_GPU_DESCRIPTOR_HANDLE srv = app->getShaderDescriptors()->getGPUHandle(srvHandle);
    cmdList->SetGraphicsRootDescriptorTable(slot, srv);
}
