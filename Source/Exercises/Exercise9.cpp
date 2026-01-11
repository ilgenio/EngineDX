#include "Globals.h"
#include "Exercise9.h"

#include "Application.h"
#include "ModuleSamplers.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleTargetDescriptors.h"
#include "ModuleCamera.h"
#include "DebugDrawPass.h"

#include "SkyboxRenderPass.h"
#include "ReadData.h"
#include "RenderTexture.h"

Exercise9::Exercise9()
{

}

Exercise9::~Exercise9()
{
}

bool Exercise9::init() 
{
    ModuleResources* resources = app->getResources();
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
    ModuleD3D12* d3d12 = app->getD3D12();

    tableDesc = descriptors->allocTable();
    debugDrawPass = std::make_unique<DebugDrawPass>(d3d12->getDevice(), d3d12->getDrawCommandQueue(), false, tableDesc.getCPUHandle(0), tableDesc.getGPUHandle(0));
    skyboxRenderPass = std::make_unique<SkyboxRenderPass>();

    skyboxRenderPass->init(false);

    cubemap = resources->createTextureFromFile(std::wstring(L"Assets/Textures/cubemap.dds"));

    if (cubemap)
    {
        tableDesc.createCubeTextureSRV(cubemap.Get(), 1);
    }

    return true;
}

bool Exercise9::cleanUp()
{
    return true;
}

void Exercise9::preRender()
{
    dd::axisTriad(ddConvert(Matrix::Identity), 0.1f, 2.0f);
}

void Exercise9::render()
{
    ModuleD3D12* d3d12 = app->getD3D12();
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
    ModuleSamplers* samplers = app->getSamplers();
    ModuleCamera* camera = app->getCamera();

    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();
    commandList->Reset(d3d12->getCommandAllocator(), nullptr);


    ID3D12DescriptorHeap* descriptorHeaps[] = { descriptors->getHeap(), samplers->getHeap() };
    commandList->SetDescriptorHeaps(2, descriptorHeaps);


    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    unsigned width = d3d12->getWindowWidth();
    unsigned height = d3d12->getWindowHeight();

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = d3d12->getRenderTargetDescriptor();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = d3d12->getDepthStencilDescriptor();

    commandList->OMSetRenderTargets(1, &rtv, false, &dsv);

    float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, LONG(width), LONG(height) };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    Matrix proj = ModuleCamera::getPerspectiveProj(float(width) / float(height));
    skyboxRenderPass->record(commandList, tableDesc.getGPUHandle(1), camera->getRot(), proj);
    debugDrawPass->record(commandList, width, height, camera->getView(), proj);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier);

    if (SUCCEEDED(commandList->Close()))
    {
        ID3D12CommandList* commandLists[] = { commandList };
        d3d12->getDrawCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);
    }
}
