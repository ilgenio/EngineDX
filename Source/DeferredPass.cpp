#include "Globals.h"

#include "DeferredPass.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleSamplers.h"

#include "GBuffer.h"
#include "ReadData.h"

DeferredPass::DeferredPass()
{
}

DeferredPass::~DeferredPass()
{
}

bool DeferredPass::init()
{
    if (!createRootSignature())
        return false;

    if (!createPSO())
        return false;

    return true;
}

void DeferredPass::render(ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS perFrameData, D3D12_GPU_DESCRIPTOR_HANDLE gbufferTable, D3D12_GPU_DESCRIPTOR_HANDLE lightsTable, D3D12_GPU_DESCRIPTOR_HANDLE iblTable)
{
    BEGIN_EVENT(commandList, "Deferred Pass");

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pso.Get());

    commandList->SetGraphicsRootConstantBufferView(SLOT_PER_FRAME_CB, perFrameData);
    commandList->SetGraphicsRootDescriptorTable(SLOT_GBUFFER_TABLE, gbufferTable);
    //commandList->SetGraphicsRootDescriptorTable(SLOT_LIGHTS_TABLE, lightsTable);
    commandList->SetGraphicsRootDescriptorTable(SLOT_IBL_TABLE, iblTable);
    commandList->SetGraphicsRootDescriptorTable(SLOT_SAMPLERS, app->getSamplers()->getGPUHandle(ModuleSamplers::LINEAR_WRAP));

    // Draw fullscreen triangle
    commandList->IASetVertexBuffers(0, 0, nullptr);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    END_EVENT(commandList);
}

bool DeferredPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[SLOT_COUNT] = {};
    CD3DX12_DESCRIPTOR_RANGE lightsTableRange, iblTableRange, gbufferTableRange;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    gbufferTableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);
    lightsTableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 4);
    iblTableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 7);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);

    rootParameters[SLOT_PER_FRAME_CB].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[SLOT_GBUFFER_TABLE].InitAsDescriptorTable(1, &gbufferTableRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[SLOT_LIGHTS_TABLE].InitAsDescriptorTable(1, &lightsTableRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[SLOT_IBL_TABLE].InitAsDescriptorTable(1, &iblTableRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[SLOT_SAMPLERS].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(SLOT_COUNT, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    rootSignature = app->getD3D12()->createRootSignature(rootSignatureDesc);

    return rootSignature;
}

bool DeferredPass::createPSO()
{
    auto dataVS = DX::ReadData(L"fullscreenVS.cso");
    auto dataPS = DX::ReadData(L"deferredPS.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0};
    psoDesc.pRootSignature = rootSignature.Get();                                                   
    psoDesc.VS = { dataVS.data(), dataVS.size() };                                                  
    psoDesc.PS = { dataPS.data(), dataPS.size() };                                                  
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;                         
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                                         
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc = {1, 0};                                                                    
    psoDesc.SampleMask = 0xffffffff;                                                                
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);                               
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         
    psoDesc.NumRenderTargets = 1;                                                                   

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}