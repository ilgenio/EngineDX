#include "Globals.h"

#include "SSAOPass.h"
#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleSamplers.h"
#include "ModuleResources.h"
#include "ModuleTargetDescriptors.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleRingBuffer.h"

#include "RenderStructs.h"
#include "GBuffer.h"

#include "ReadData.h"

#include <random>

#define KERNEL_RADIUS 0.15f

#define GROUP_SIZE_X 8  
#define GROUP_SIZE_Y 8

SSAOPass::SSAOPass()
{
    createRootSignature();
    createPSO();
    createBlurRootSignature();
    createBlurPSO();
    createKernel();
}


SSAOPass::~SSAOPass()
{

}

void SSAOPass::render(ID3D12GraphicsCommandList* commandList, const RenderData& renderData)
{
    BEGIN_EVENT(commandList, "SSAO Pass");

    renderSSAO(commandList, renderData);
    blurSSAO(commandList);

    END_EVENT(commandList);
}

bool SSAOPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[ROOT_COUNT] = {};

    CD3DX12_DESCRIPTOR_RANGE depthSrvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    CD3DX12_DESCRIPTOR_RANGE normalSrvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    CD3DX12_DESCRIPTOR_RANGE sampRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);

    rootParameters[ROOT_PARAMS].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[ROOT_DEPTH_SRV].InitAsDescriptorTable(1, &depthSrvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[ROOT_NORMAL_SRV].InitAsDescriptorTable(1, &normalSrvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[ROOT_SAMPLERS].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(ROOT_COUNT, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    rootSignature = app->getD3D12()->createRootSignature(rootSignatureDesc);

    return rootSignature != nullptr;
}

bool SSAOPass::createPSO()
{
    // Implementation for creating pipeline state object
    auto dataVS = DX::ReadData(L"fullscreenVS.cso");
    auto dataPS = DX::ReadData(L"ssaoPS.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { nullptr, 0};
    psoDesc.pRootSignature = rootSignature.Get();                                                   
    psoDesc.VS = { dataVS.data(), dataVS.size() };                                                  
    psoDesc.PS = { dataPS.data(), dataPS.size() };                                                  
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;                         
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16_FLOAT; // Assuming the SSAO result is stored in a single channel 16-bit float format                           
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
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

bool SSAOPass::createBlurRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameter[NUM_ROOT_PARAMETERS_BLUR] = {};

    CD3DX12_DESCRIPTOR_RANGE inputRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    CD3DX12_DESCRIPTOR_RANGE outputRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    CD3DX12_DESCRIPTOR_RANGE sampRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);


    rootParameter[ROOT_BLUR_CONSTANTS].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameter[ROOT_BLUR_INPUT].InitAsDescriptorTable(1, &inputRange, D3D12_SHADER_VISIBILITY_ALL);
    rootParameter[ROOT_BLUR_OUTPUT].InitAsDescriptorTable(1, &outputRange, D3D12_SHADER_VISIBILITY_ALL);
    rootParameter[ROOT_BLUR_SAMPLERS].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_ALL);

    rootSignatureDesc.Init(NUM_ROOT_PARAMETERS_BLUR, rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    blurRootSignature = app->getD3D12()->createRootSignature(rootSignatureDesc);

    return blurRootSignature != nullptr;
}

bool SSAOPass::createBlurPSO()
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    auto dataCS = DX::ReadData(L"ssaoBlurCS.cso");

    psoDesc.pRootSignature = blurRootSignature.Get();
    psoDesc.CS = { dataCS.data(), dataCS.size() };

    if (FAILED(app->getD3D12()->getDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&blurPSO))))
    {
        return false;
    }

    return true;
}

void SSAOPass::createKernel()
{
    std::uniform_real_distribution<float> randoms(0.0f, 1.0f);
    std::default_random_engine generator;

    auto lerp = [](float a, float b, float f) { return a + f * (b - a); };

    for (UINT i = 0; i < KERNEL_SIZE; ++i)
    {
        Vector3 dir;
        dir.x = randoms(generator)*2.0f-1.0f;
        dir.y = randoms(generator)*2.0f-1.0f;
        dir.z = randoms(generator);

        dir.Normalize();

        float scale = float(i) / float(KERNEL_SIZE);

        scale = lerp(0.1f, 1.0f, scale*scale); 
        
        dir *= scale*KERNEL_RADIUS; // scale the direction vector

        params.kernel[i] = Vector4(dir.x, dir.y, dir.z, 0.0f);
    }
}

void SSAOPass::resize(UINT width, UINT height)
{
    if(width == this->width && height == this->height)
        return;

    this->width = width;
    this->height = height;
    
    ModuleResources* resources = app->getResources();

    resources->deferRelease(ssaoResult);
    resources->deferRelease(blur0);
    resources->deferRelease(blur1);

    ssaoResult = resources->createRenderTarget(DXGI_FORMAT_R16_FLOAT, width, height, 1, Vector4::Zero, "ssaoResult"); 
    blur0 = resources->createUnorderedAccessTexture2D(DXGI_FORMAT_R16_FLOAT, width, height, "ssaoBlur0");
    blur1 = resources->createUnorderedAccessTexture2D(DXGI_FORMAT_R16_FLOAT, width, height, "ssaoBlur1");


    ModuleTargetDescriptors* targetDescriptors = app->getTargetDescriptors();
    rtv = targetDescriptors->createRTV(ssaoResult.Get());

    ModuleShaderDescriptors* shaderDescriptors = app->getShaderDescriptors();
    srvDesc = shaderDescriptors->allocTable();
    srvDesc.createTextureSRV(ssaoResult.Get(), 0);
    srvDesc.createTextureSRV(blur0.Get(), 1);
    srvDesc.createTextureSRV(blur1.Get(), 2);
    srvDesc.createTextureUAV(blur0.Get(), 3);
    srvDesc.createTextureUAV(blur1.Get(), 4);
}

void SSAOPass::renderSSAO(ID3D12GraphicsCommandList *commandList, const RenderData &renderData)
{
    BEGIN_EVENT(commandList, "SSAO Render Pass");

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pso.Get());

    params.proj = renderData.proj.Transpose();
    params.view = renderData.view.Transpose();
    params.frameIndex++;
    params.width = width;
    params.height = height;
    params.rangeDistance = 0.25f; // Example kernel radius, can be adjusted or made dynamic
    params.bias = 0.0001f; // Example bias, can be adjusted or made dynamic
    ModuleRingBuffer* ringBuffer = app->getRingBuffer();

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtv.getCPUHandle();
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, float(renderData.width), float(renderData.height), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, LONG(renderData.width), LONG(renderData.height) };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(ssaoResult.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    commandList->SetGraphicsRootConstantBufferView(ROOT_PARAMS, ringBuffer->alloc(&params));
    commandList->SetGraphicsRootDescriptorTable(ROOT_DEPTH_SRV, renderData.gBuffer.getSrvTableDesc().getGPUHandle(GBuffer::BUFFER_DEPTH));
    commandList->SetGraphicsRootDescriptorTable(ROOT_NORMAL_SRV, renderData.gBuffer.getSrvTableDesc().getGPUHandle(GBuffer::BUFFER_NORMAL_METALLIC_ROUGHNESS));
    commandList->SetGraphicsRootDescriptorTable(ROOT_SAMPLERS, app->getSamplers()->getGPUHandle(ModuleSamplers::LINEAR_WRAP));

    // Draw fullscreen triangle
    commandList->IASetVertexBuffers(0, 0, nullptr);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->DrawInstanced(3, 1, 0, 0);

    END_EVENT(commandList);
}

void SSAOPass::blurSSAO(ID3D12GraphicsCommandList* commandList)
{
    BEGIN_EVENT(commandList, "SSAO Blur Pass");

    commandList->SetComputeRootSignature(blurRootSignature.Get());
    commandList->SetPipelineState(blurPSO.Get());

    ModuleRingBuffer* ringBuffer = app->getRingBuffer();

    // Horizontal blur

    BlurConstants blurData = {};
    blurData.directionX = 1;
    blurData.directionY = 0;
    blurData.width = width;
    blurData.height = height;

    commandList->SetComputeRootConstantBufferView(ROOT_BLUR_CONSTANTS, ringBuffer->alloc(&blurData));
    commandList->SetComputeRootDescriptorTable(ROOT_BLUR_INPUT, srvDesc.getGPUHandle(0));
    commandList->SetComputeRootDescriptorTable(ROOT_BLUR_OUTPUT, srvDesc.getGPUHandle(3));
    commandList->SetComputeRootDescriptorTable(ROOT_BLUR_SAMPLERS, app->getSamplers()->getGPUHandle(ModuleSamplers::LINEAR_WRAP));

    CD3DX12_RESOURCE_BARRIER barriers[2];

    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(ssaoResult.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(blur0.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(2, barriers);


    UINT dispatchX = getDivisedSize(width, GROUP_SIZE_X);
    UINT dispatchY = getDivisedSize(height, GROUP_SIZE_Y);

    commandList->Dispatch(dispatchX, dispatchY, 1);

    // Vertical blur

    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(blur0.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(blur1.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(2, &barriers[0]);

    blurData.directionX = 0;
    blurData.directionY = 1;
    commandList->SetComputeRootConstantBufferView(ROOT_BLUR_CONSTANTS, ringBuffer->alloc(&blurData));
    commandList->SetComputeRootDescriptorTable(ROOT_BLUR_INPUT, srvDesc.getGPUHandle(1));
    commandList->SetComputeRootDescriptorTable(ROOT_BLUR_OUTPUT, srvDesc.getGPUHandle(4));

    commandList->Dispatch(dispatchX, dispatchY, 1);

    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(blur1.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barriers[0]);

    END_EVENT(commandList);
}
