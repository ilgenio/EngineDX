#include "Globals.h"

#include "DecalPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleSamplers.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleRingBuffer.h"

#include "DecalCubeMesh.h"
#include "GBuffer.h"
#include "RenderStructs.h"
#include "Scene.h"

#include "MathUtils.h"


#include "Decal.h"

#include "ReadData.h"

DecalPass::DecalPass()
{
    decalCubeMesh = std::make_unique<DecalCubeMesh>();

    createRootSignature();
    createPSO();
}

DecalPass::~DecalPass()
{
}

void DecalPass::render(ID3D12GraphicsCommandList* commandList, std::span<const std::shared_ptr<Decal> > decals, const RenderData& renderData)
{
    ModuleRingBuffer* ringBuffer = app->getRingBuffer();
    ModuleSamplers* samplers = app->getSamplers();

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pso.Get());

    commandList->SetGraphicsRootDescriptorTable(SLOT_SAMPLERS, samplers->getGPUHandle(ModuleSamplers::LINEAR_WRAP));
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // TODO G-buffer render targets already transitioned RTV must be set

    for (const auto& decal : decals)
    {
        const Matrix& model = decal->getTransform();

        Matrix mvp = (model*renderData.viewProj).Transpose();

        commandList->SetGraphicsRoot32BitConstants(SLOT_MVP_MATRIX, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);

        Constants constants     = {};
        constants.projection    = renderData.proj;
        constants.invView       = renderData.invView;
        constants.invModel      = invertAffineTransform(model);

        commandList->SetGraphicsRootConstantBufferView(SLOT_DECAL_CONSTANTS, ringBuffer->alloc(&constants, sizeof(constants)));
        commandList->SetGraphicsRootDescriptorTable(SLOT_TEXTURES_TABLE, decal->getTextureTableDesc().getGPUHandle());

        decalCubeMesh->draw(commandList);
    }
}

bool DecalPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[SLOT_COUNT] = {};
    CD3DX12_DESCRIPTOR_RANGE texturesTableRange;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    texturesTableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);

    rootParameters[SLOT_MVP_MATRIX].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[SLOT_DECAL_CONSTANTS].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[SLOT_TEXTURES_TABLE].InitAsDescriptorTable(1, &texturesTableRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[SLOT_SAMPLERS].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(SLOT_COUNT, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    rootSignature = app->getD3D12()->createRootSignature(rootSignatureDesc);

    return rootSignature != nullptr;
}

bool DecalPass::createPSO()
{
    // Implementation for creating pipeline state object
    auto dataVS = DX::ReadData(L"decalVS.cso");
    auto dataPS = DX::ReadData(L"decalPS.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout =  decalCubeMesh->getInputLayoutDesc();
    psoDesc.pRootSignature = rootSignature.Get();                            
    psoDesc.VS = { dataVS.data(), dataVS.size() };                           
    psoDesc.PS = { dataPS.data(), dataPS.size() };                           
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;  
    psoDesc.SampleDesc = { 1, 0};                                            
    psoDesc.SampleMask = 0xffffffff;                                         
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);        
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;                    
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                   

    psoDesc.NumRenderTargets = 3;
    psoDesc.RTVFormats[0] = GBuffer::getRTFormat(GBuffer::BUFFER_ALBEDO);

    // Enable writing to RGB 
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = ( D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN   | D3D12_COLOR_WRITE_ENABLE_BLUE );

    psoDesc.RTVFormats[1] = GBuffer::getRTFormat(GBuffer::BUFFER_NORMAL_METALLIC_ROUGHNESS);

    // Enable writing to RGB 
    psoDesc.BlendState.RenderTarget[1].RenderTargetWriteMask = 0 ; //( D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN )  | D3D12_COLOR_WRITE_ENABLE_BLUE );

    psoDesc.RTVFormats[2] = GBuffer::getRTFormat(GBuffer::BUFFER_EMISSIVE_AO);

    // Enable writing to Alpha only (AO)
    psoDesc.BlendState.RenderTarget[2].RenderTargetWriteMask = 0; //D3D12_COLOR_WRITE_ENABLE_ALPHA;

    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso))); 
}