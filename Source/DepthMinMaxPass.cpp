#include "Globals.h"

#include "DepthMinMaxPass.h"

#include "Application.h"
#include "ModuleResources.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleD3D12.h"
#include "ModuleCamera.h"
#include "ModuleRingBuffer.h"

#include "GBuffer.h"
#include "RenderStructs.h"

#include "ReadData.h"

#define GROUP_SIZE_X 8
#define GROUP_SIZE_Y 8

DepthMinMaxPass::DepthMinMaxPass()
{
    createMinMaxRS();
    createMinMaxPSO();

    createBuildRS();
    createBuildPSO();

    vpBuffer = app->getResources()->createUnorderedAccessBuffer(sizeof(Matrix), "DepthMinMax VP Buffer");
}

DepthMinMaxPass::~DepthMinMaxPass()
{
}

void DepthMinMaxPass::resize(UINT width, UINT height)
{
    if(this->width == width && this->height == height)
        return;

    this->width = width;
    this->height = height;

    UINT numTilesX = getDivisedSize(width, GROUP_SIZE_X);
    UINT numTilesY = getDivisedSize(height, GROUP_SIZE_Y);

    // Create or resize depthMinMaxTexture0 and depthMinMaxTexture1 to numTilesX x numTilesY

    ModuleResources* resources = app->getResources();

    resources->deferRelease(depthMinMaxTexture[0]);
    resources->deferRelease(depthMinMaxTexture[1]);

    depthMinMaxTexture[0] = resources->createUnorderedAccessTexture2D(numTilesX, numTilesY, DXGI_FORMAT_R32G32_FLOAT, "DepthMinMax0");
    depthMinMaxTexture[1] = resources->createUnorderedAccessTexture2D(numTilesX, numTilesY, DXGI_FORMAT_R32G32_FLOAT, "DepthMinMax1");

    shaderTableDesc = app->getShaderDescriptors()->allocTable();

    // Create SRV for input depth buffer and UAVs for ping-pong textures

    shaderTableDesc.createTextureSRV(depthMinMaxTexture[0].Get(), 0); // Assuming the depth buffer SRV is at slot 1
    shaderTableDesc.createTextureSRV(depthMinMaxTexture[1].Get(), 1); // Assuming the depth buffer SRV is at slot 1

    shaderTableDesc.createTextureUAV(depthMinMaxTexture[1].Get(), 2); // UAV for ping-pong texture 0 at slot 3
    shaderTableDesc.createTextureUAV(depthMinMaxTexture[0].Get(), 3); // UAV for ping-pong texture 1 at slot 4
}

void DepthMinMaxPass::record(ID3D12GraphicsCommandList* commandList, const Vector3& lightDir, const RenderData& renderData)
{
    BEGIN_EVENT(commandList, "DepthMinMax Pass");

    UINT numTilesX = getDivisedSize(width, GROUP_SIZE_X);
    UINT numTilesY = getDivisedSize(height, GROUP_SIZE_Y);

    commandList->SetPipelineState(minMaxPSO.Get());
    commandList->SetComputeRootSignature(minMaxRS.Get());

    MinMaxConstants constants = {};
    constants.width = width;
    constants.height = height;
    constants.inputIsDepth = TRUE; // First iteration with depth buffer as input

    commandList->SetComputeRootDescriptorTable(MINMAX_ROOTPARAM_INPUT_DEPTH, renderData.gBuffer.getSrvTableDesc().getGPUHandle(GBuffer::BUFFER_DEPTH)); 

    UINT pingPongIndex = 0;

    while(true)
    {
        UINT transitionToUAVIndex = (pingPongIndex + 1) % 2; // The texture index that will be used as UAV in this iteration

        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(depthMinMaxTexture[transitionToUAVIndex].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->ResourceBarrier(1, &barrier);

        UINT srvIndex = pingPongIndex % 2;  // 0 for first iteration, then alternates between 0 and 1
        UINT uavIndex = 2 + srvIndex;       // 2 for first iteration, then alternates between 2 and 3

        commandList->SetComputeRoot32BitConstants(MINMAX_ROOTPARAM_CONSTANTS, sizeof(MinMaxConstants) / 4, &constants, 0);
        commandList->SetComputeRootDescriptorTable(MINMAX_ROOTPARAM_INPUT_MINMAX, shaderTableDesc.getGPUHandle(srvIndex)); 
        commandList->SetComputeRootDescriptorTable(MINMAX_ROOTPARAM_OUTPUT, shaderTableDesc.getGPUHandle(uavIndex)); 

        commandList->Dispatch(numTilesX, numTilesY, 1);

        barrier = CD3DX12_RESOURCE_BARRIER::Transition(depthMinMaxTexture[transitionToUAVIndex].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &barrier);

        // Dispatch compute shader with enough thread groups to cover numTilesX x numTilesY

        ++pingPongIndex;

        constants.width = numTilesX;
        constants.height = numTilesY;
        constants.inputIsDepth = FALSE; // Subsequent iterations with min-max texture as input

        if(numTilesX == 1 && numTilesY == 1)
            break; // If we've reduced to a single tile, we can stop

        // Update numTilesX and numTilesY for the next level of reduction
        numTilesX = getDivisedSize(numTilesX, GROUP_SIZE_X);
        numTilesY = getDivisedSize(numTilesY, GROUP_SIZE_Y);
    }
    END_EVENT(commandList);

    BEGIN_EVENT(commandList, "Build Shadow VP Pass");


    // This is the result index
    finalSrvIndex = pingPongIndex % 2; 

    // Generate the final VP matrix using the final min-max result

    commandList->SetPipelineState(buildPSO.Get());
    commandList->SetComputeRootSignature(buildRS.Get());

    ModuleCamera* camera = app->getCamera();
    ModuleRingBuffer* ringBuffer = app->getRingBuffer();

    float aspectRatio = float(width) / float(height);

    BuildConstants buildConstants = {};
    buildConstants.projection = ModuleCamera::getPerspectiveProj(aspectRatio).Transpose();
    buildConstants.invView = camera->getCamera().Transpose();
    buildConstants.lightDir = lightDir; 
    buildConstants.aspectRatio = aspectRatio;
    buildConstants.fov = XM_PIDIV4;

    commandList->SetComputeRootConstantBufferView(0, ringBuffer->alloc(&buildConstants));

    commandList->SetComputeRootDescriptorTable(BUILD_ROOTPARAM_INPUT_MINMAX, shaderTableDesc.getGPUHandle(finalSrvIndex));
    commandList->SetComputeRootUnorderedAccessView(BUILD_ROOTPARAM_OUTPUT_VP, vpBuffer->GetGPUVirtualAddress()); 

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(vpBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(1, &barrier);

    commandList->Dispatch(1, 1, 1);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(vpBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier);

    END_EVENT(commandList);
}

bool DepthMinMaxPass::createMinMaxRS()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[MINMAX_ROOTPARAM_COUNT] = {};

    CD3DX12_DESCRIPTOR_RANGE ranges[3];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    rootParameters[MINMAX_ROOTPARAM_CONSTANTS].InitAsConstants(sizeof(MinMaxConstants) / 4, 0); // size in 32-bit values, register b0    
    rootParameters[MINMAX_ROOTPARAM_INPUT_DEPTH].InitAsDescriptorTable(1, &ranges[0]); // SRV for input depth texture
    rootParameters[MINMAX_ROOTPARAM_INPUT_MINMAX].InitAsDescriptorTable(1, &ranges[1]); // SRV for input min-max texture
    rootParameters[MINMAX_ROOTPARAM_OUTPUT].InitAsDescriptorTable(1, &ranges[2]); // UAV for output min-max texture

    rootSignatureDesc.Init(MINMAX_ROOTPARAM_COUNT, rootParameters);

    minMaxRS = app->getD3D12()->createRootSignature(rootSignatureDesc);

    return minMaxRS != nullptr;
}

bool DepthMinMaxPass::createMinMaxPSO()
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    auto dataCS = DX::ReadData(L"minMaxDepthCS.cso");

    psoDesc.pRootSignature = minMaxRS.Get();
    psoDesc.CS = { dataCS.data(), dataCS.size() };

    if (FAILED(app->getD3D12()->getDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&minMaxPSO))))
    {
        return false;
    }

    return true;
}

bool DepthMinMaxPass::createBuildRS()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[BUILD_ROOTPARAM_COUNT] = {};

    CD3DX12_DESCRIPTOR_RANGE range = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    rootParameters[BUILD_ROOTPARAM_CONSTANTS].InitAsConstantBufferView(0); // Register b0 for constants
    rootParameters[BUILD_ROOTPARAM_INPUT_MINMAX].InitAsDescriptorTable(1, &range); // SRV for input min-max texture
    rootParameters[BUILD_ROOTPARAM_OUTPUT_VP].InitAsUnorderedAccessView(0);

    rootSignatureDesc.Init(BUILD_ROOTPARAM_COUNT, rootParameters);

    buildRS = app->getD3D12()->createRootSignature(rootSignatureDesc);

    return buildRS != nullptr;
}

bool DepthMinMaxPass::createBuildPSO()
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    auto dataCS = DX::ReadData(L"buildShadowVPCS.cso");

    psoDesc.pRootSignature = buildRS.Get();
    psoDesc.CS = { dataCS.data(), dataCS.size() };

    if (FAILED(app->getD3D12()->getDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&buildPSO))))
    {
        return false;
    }

    return true;
}