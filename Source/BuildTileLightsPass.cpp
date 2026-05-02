#include "Globals.h"

#include "BuildTileLightsPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleCamera.h"

#include "ModuleRingBuffer.h"
#include "ModuleResources.h"
#include "ModulePerFrameBuffer.h"

#include "Light.h"
#include "MathUtils.h"

#include "ReadData.h"

#define MAX_LIGHTS_PER_TILE 1024
#define TILE_SIZE 16

namespace
{
    UINT getNumTiles(UINT size)
    {
        return (size + TILE_SIZE - 1) / TILE_SIZE;
    }
}

BuildTileLightsPass::BuildTileLightsPass()
{
    createRootSignature();
    createPSO();
}

BuildTileLightsPass::~BuildTileLightsPass()
{

}

void BuildTileLightsPass::resize(UINT width, UINT height)
{
    if (width == this->width && height == this->height)
        return;

    this->width = width;
    this->height = height;

    ModuleResources* resources = app->getResources();

    UINT totalTiles = getNumTiles(width) * getNumTiles(height);


    for (UINT i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        resources->deferRelease(pointLists[i]);
        resources->deferRelease(spotLists[i]);
        pointLists[i] = resources->createUnorderedAccessBuffer(totalTiles * sizeof(UINT) * MAX_LIGHTS_PER_TILE, "Point List Buffer");
        spotLists[i] = resources->createUnorderedAccessBuffer(totalTiles * sizeof(UINT) * MAX_LIGHTS_PER_TILE, "Spot List Buffer");
    }
}

void BuildTileLightsPass::record(ID3D12GraphicsCommandList* commandList, int width, int height, const Matrix& view, const Matrix& proj,
                                 std::span<Point*> pointLights, std::span<Spot*> spotLights, 
                                 D3D12_GPU_DESCRIPTOR_HANDLE depthSRV)
{
    generateSphereData(pointLights, spotLights);

    BEGIN_EVENT(commandList, "BuildTileLightsPass");

    commandList->SetPipelineState(pso.Get());
    commandList->SetComputeRootSignature(rootSignature.Get());

    ModuleCamera* camera = app->getCamera();
    ModulePerFrameBuffer* perFrameBuffer = app->getPerFrameBuffer();
    UINT outputIndex = app->getD3D12()->getCurrentBackBufferIdx();

    Constants constants = {};
    constants.view           = view.Transpose();
    constants.proj           = proj.Transpose();
    constants.width          = width;
    constants.height         = height;
    constants.numPointLights = UINT(pointLights.size());
    constants.numSpotLights  = UINT(spotLights.size());

    D3D12_GPU_VIRTUAL_ADDRESS constantsAddress = perFrameBuffer->alloc(&constants);

    D3D12_GPU_VIRTUAL_ADDRESS pointSphereAddress = perFrameBuffer->alloc(pointSphereData.data(), pointSphereData.size());
    D3D12_GPU_VIRTUAL_ADDRESS spotSphereAddress = perFrameBuffer->alloc(spotSphereData.data(), spotSphereData.size());

    commandList->SetComputeRootConstantBufferView(ROOTPARAM_CONSTANTS, constantsAddress);
    commandList->SetComputeRootDescriptorTable(ROOTPARAM_DEPTH_TABLE, depthSRV);
    commandList->SetComputeRootShaderResourceView(ROOTPARAM_POINT_SPHERE_SRV, pointSphereAddress);
    commandList->SetComputeRootShaderResourceView(ROOTPARAM_SPOT_SPHERE_SRV, spotSphereAddress);
    commandList->SetComputeRootUnorderedAccessView(ROOTPARAM_POINT_LIST_UAV, pointLists[outputIndex]->GetGPUVirtualAddress());
    commandList->SetComputeRootUnorderedAccessView(ROOTPARAM_SPOT_LIST_UAV, spotLists[outputIndex]->GetGPUVirtualAddress());


    CD3DX12_RESOURCE_BARRIER barrier[2];
    barrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(pointLists[outputIndex].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    barrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(spotLists[outputIndex].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(2, &barrier[0]);

    perFrameBuffer->submitCopy(commandList);

    UINT dispatchX = getNumTiles(width); 
    UINT dispatchY = getNumTiles(height);

    commandList->Dispatch(dispatchX, dispatchY, 1);

    barrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(pointLists[outputIndex].Get() , D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    barrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(spotLists[outputIndex].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(2, &barrier[0]);

    END_EVENT(commandList);
}

bool BuildTileLightsPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[ROOTPARAM_COUNT] = {};

    CD3DX12_DESCRIPTOR_RANGE depthRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    rootParameters[ROOTPARAM_CONSTANTS].InitAsConstantBufferView(0);
    rootParameters[ROOTPARAM_DEPTH_TABLE].InitAsDescriptorTable(1, &depthRange);
    rootParameters[ROOTPARAM_POINT_SPHERE_SRV].InitAsShaderResourceView(1);
    rootParameters[ROOTPARAM_SPOT_SPHERE_SRV].InitAsShaderResourceView(2);
    rootParameters[ROOTPARAM_POINT_LIST_UAV].InitAsUnorderedAccessView(0);
    rootParameters[ROOTPARAM_SPOT_LIST_UAV].InitAsUnorderedAccessView(1);

    rootSignatureDesc.Init(ROOTPARAM_COUNT, rootParameters);

    rootSignature = app->getD3D12()->createRootSignature(rootSignatureDesc);

    return rootSignature != nullptr;
}

bool BuildTileLightsPass::createPSO()
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    auto dataCS = DX::ReadData(L"tileCullingCS.cso");

    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.CS = { dataCS.data(), dataCS.size() };

    if (FAILED(app->getD3D12()->getDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso))))
    {
        return false;
    }

    return true;
}

void BuildTileLightsPass::generateSphereData(std::span<Point*> pointLights, std::span<Spot*> spotLights)
{
    pointSphereData.clear();
    spotSphereData.clear();

    pointSphereData.reserve(pointLights.size());
    spotSphereData.reserve(spotLights.size());

    for (const Point* point : pointLights)
    {
        pointSphereData.emplace_back(point->Lp.x, point->Lp.y, point->Lp.z, sqrtf(point->sqRadius));
    }

    for (const Spot* spot : spotLights)
    {
        Vector4 boundingSphere = getBoundingSphere(*spot); 
        spotSphereData.push_back(boundingSphere);
    }
}

D3D12_GPU_VIRTUAL_ADDRESS BuildTileLightsPass::getPointListAddress() const
{
    UINT outputIndex = app->getD3D12()->getCurrentBackBufferIdx();

    return pointLists[outputIndex]->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS BuildTileLightsPass::getSpotListAddress() const
{
    UINT outputIndex = app->getD3D12()->getCurrentBackBufferIdx();

    return spotLists[outputIndex]->GetGPUVirtualAddress();
}

