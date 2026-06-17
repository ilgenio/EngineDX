#include "Globals.h"
#include "TAAPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleRingBuffer.h"

#include "ReadData.h"
#include "RenderStructs.h"
#include "RenderTexture.h"


TAAPass::TAAPass()
{
    createRootSignature();
    createPSO();
}

TAAPass::~TAAPass()
{
    // Destructor implementation
}

void TAAPass::render(ID3D12GraphicsCommandList* commandList, const RenderData& renderData, D3D12_GPU_DESCRIPTOR_HANDLE currentFrame)
{
    BEGIN_EVENT(commandList, "TAA Pass");

    CD3DX12_RESOURCE_BARRIER toUAV = CD3DX12_RESOURCE_BARRIER::Transition(taaResult.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(1, &toUAV);

    commandList->SetComputeRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pso.Get());

    TAAConstants constants = {};
    constants.width         = width;
    constants.height        = height;
    constants.proj          = renderData.proj.Transpose();
    constants.invView       = renderData.invView.Transpose();
    constants.prevViewProj  = renderData.prevViewProj.Transpose();
    constants.prevWidth     = renderData.prevWidth;
    constants.prevHeight    = renderData.prevHeight;
    constants.jitterX       = renderData.proj._31;
    constants.jitterY       = renderData.proj._32;

    ModuleRingBuffer* ringBuffer = app->getRingBuffer();

    commandList->SetComputeRootConstantBufferView(ROOT_TAA_CONSTANTS, ringBuffer->alloc(&constants));
    commandList->SetComputeRootDescriptorTable(ROOT_TAA_PREVIOUS, currentFrame);
    commandList->SetComputeRootDescriptorTable(ROOT_TAA_DEPTH, renderData.gBuffer.getSrvTableDesc().getGPUHandle(GBuffer::BUFFER_DEPTH));
    commandList->SetComputeRootDescriptorTable(ROOT_TAA_CURRENT, tableDesc.getGPUHandle(1));

    UINT numGroupsX = getDivisedSize(width, 8);
    UINT numGroupsY = getDivisedSize(height, 8);

    commandList->Dispatch(numGroupsX, numGroupsY, 1);

    CD3DX12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(taaResult.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &toSRV);

    END_EVENT(commandList);
}

bool TAAPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameter[NUM_ROOT_PARAMETERS_TAA] = {};

    CD3DX12_DESCRIPTOR_RANGE sourceRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    CD3DX12_DESCRIPTOR_RANGE depthRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

    CD3DX12_DESCRIPTOR_RANGE uavRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    rootParameter[ROOT_TAA_CONSTANTS].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameter[ROOT_TAA_PREVIOUS].InitAsDescriptorTable(1, &sourceRange, D3D12_SHADER_VISIBILITY_ALL);
    rootParameter[ROOT_TAA_DEPTH].InitAsDescriptorTable(1, &depthRange, D3D12_SHADER_VISIBILITY_ALL);
    rootParameter[ROOT_TAA_CURRENT].InitAsDescriptorTable(1, &uavRange, D3D12_SHADER_VISIBILITY_ALL);
    rootSignatureDesc.Init(NUM_ROOT_PARAMETERS_TAA, rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    rootSignature = app->getD3D12()->createRootSignature(rootSignatureDesc);

    return rootSignature != nullptr;
}

bool TAAPass::createPSO()
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    auto dataCS = DX::ReadData(L"taaCS.cso");

    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.CS = { dataCS.data(), dataCS.size() };

    if (FAILED(app->getD3D12()->getDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso))))
    {
        return false;
    }

    return true;
}

void TAAPass::resize(UINT width, UINT height)
{
    if(width == this->width && height == this->height)
        return;

    this->width = width;
    this->height = height;

    ModuleResources* resources = app->getResources();

    resources->deferRelease(taaResult);

    taaResult = resources->createUnorderedAccessTexture2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, "taaResult");

    ModuleShaderDescriptors* shaderDescriptors = app->getShaderDescriptors();
    tableDesc = shaderDescriptors->allocTable();
    tableDesc.createTextureSRV(taaResult.Get(), 0);
    tableDesc.createTextureUAV(taaResult.Get(), 1);
}