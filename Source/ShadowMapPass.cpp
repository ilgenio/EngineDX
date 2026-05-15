#include "Globals.h"

#include "ShadowMapPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ModuleTargetDescriptors.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleRingBuffer.h"
#include "ModuleSamplers.h"

#include "RenderStructs.h"
#include "Scene.h"

#include "ReadData.h"

#include "Mesh.h"

#define SHADOW_MAP_SIZE 1 << 13

#define GROUP_SIZE_X 8  
#define GROUP_SIZE_Y 8

ShadowMapPass::ShadowMapPass()
{
    createRootSignature();
    createPSO();
    createBlurRootSignature();
    createBlurPSO();
    createShadowMapResources();
}

ShadowMapPass::~ShadowMapPass()
{

}

void ShadowMapPass::render(ID3D12GraphicsCommandList* commandList, std::span<const RenderMesh> meshes, const RenderData& renderData)
{
    renderShadowMap(commandList, meshes, renderData);
    blurShadowMap(commandList);
}


void ShadowMapPass::renderShadowMap(ID3D12GraphicsCommandList* commandList, std::span<const RenderMesh> meshes, const RenderData& renderData)
{
    BEGIN_EVENT(commandList, "ShadowMap Pass");

    ModuleRingBuffer* ringBuffer = app->getRingBuffer();
    ModuleSamplers* samplers = app->getSamplers();

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pso.Get());

    D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsvDesc.getCPUHandle();
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvDesc.getCPUHandle();

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(moments.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    commandList->ClearDepthStencilView(dsvDesc.getCPUHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, float(SHADOW_MAP_SIZE), float(SHADOW_MAP_SIZE), 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, LONG(SHADOW_MAP_SIZE), LONG(SHADOW_MAP_SIZE) };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (const RenderMesh& mesh : meshes)
    {
        Matrix model;

        if (mesh.numJoints > 0 || mesh.numMorphTargets > 0) // skinned mesh
        {
            // Note: Skinned mesh has already bee transformed in the skinning pass 
            model = Matrix::Identity;

            D3D12_VERTEX_BUFFER_VIEW vertexBufferView = mesh.mesh->getVertexBufferView();
            vertexBufferView.BufferLocation = renderData.skinningBuffer + mesh.skinningOffset;

            commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        }
        else // rigid mesh
        {
            model = mesh.transform.Transpose();

            commandList->IASetVertexBuffers(0, 1, &mesh.mesh->getVertexBufferView());
        }

        commandList->SetGraphicsRoot32BitConstants(ROOT_SHADOW_MODEL, sizeof(Matrix) / sizeof(UINT32), &model, 0);
        commandList->SetGraphicsRootConstantBufferView(ROOT_SHADOW_VP, renderData.shadowViewProjBuffer);

        if (mesh.mesh->getNumIndices() > 0) // indexed draw
        {
            commandList->IASetIndexBuffer(&mesh.mesh->getIndexBufferView());
            commandList->DrawIndexedInstanced(mesh.mesh->getNumIndices(), 1, 0, 0, 0);
        }
        else if (mesh.mesh->getNumVertices() > 0) // non-indexed draw
        {
            commandList->DrawInstanced(mesh.mesh->getNumVertices(), 1, 0, 0);
        }
    }

    END_EVENT(commandList);
}

void ShadowMapPass::blurShadowMap(ID3D12GraphicsCommandList* commandList)
{
    BEGIN_EVENT(commandList, "ShadowMap Blur Pass");

    commandList->SetComputeRootSignature(blurRootSignature.Get());
    commandList->SetPipelineState(blurPSO.Get());

    ModuleRingBuffer* ringBuffer = app->getRingBuffer();

    // Horizontal blur

    BlurConstants blurData = {};
    blurData.directionX = 1;
    blurData.directionY = 0;
    blurData.width = SHADOW_MAP_SIZE;
    blurData.height = SHADOW_MAP_SIZE;
    blurData.sigma = 3;

    commandList->SetComputeRootConstantBufferView(ROOT_BLUR_CONSTANTS, ringBuffer->alloc(&blurData));
    commandList->SetComputeRootDescriptorTable(ROOT_BLUR_INPUT, srvDesc.getGPUHandle(0));
    commandList->SetComputeRootDescriptorTable(ROOT_BLUR_OUTPUT, srvDesc.getGPUHandle(2));
    commandList->SetComputeRootDescriptorTable(ROOT_BLUR_SAMPLERS, app->getSamplers()->getGPUHandle(ModuleSamplers::LINEAR_WRAP));

    CD3DX12_RESOURCE_BARRIER barriers[2];

    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(moments.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(momentsBlur0.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(2, &barriers[0]);

    UINT dispatchX = getDivisedSize(SHADOW_MAP_SIZE, GROUP_SIZE_X);
    UINT dispatchY = getDivisedSize(SHADOW_MAP_SIZE, GROUP_SIZE_Y);

    commandList->Dispatch(dispatchX, dispatchY, 1);

    // Vertical blur

    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(momentsBlur0.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(momentsBlur1.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->ResourceBarrier(2, &barriers[0]);

    blurData.directionX = 0;
    blurData.directionY = 1;
    commandList->SetComputeRootConstantBufferView(ROOT_BLUR_CONSTANTS, ringBuffer->alloc(&blurData));
    commandList->SetComputeRootDescriptorTable(ROOT_BLUR_INPUT, srvDesc.getGPUHandle(1));
    commandList->SetComputeRootDescriptorTable(ROOT_BLUR_OUTPUT, srvDesc.getGPUHandle(3));

    commandList->Dispatch(dispatchX, dispatchY, 1);

    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(momentsBlur1.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barriers[0]);

    END_EVENT(commandList);
}

bool ShadowMapPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameter[NUM_ROOT_PARAMETERS_SHADOW] = {};

    rootParameter[ROOT_SHADOW_MODEL].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameter[ROOT_SHADOW_VP].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    rootSignatureDesc.Init(NUM_ROOT_PARAMETERS_SHADOW, rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    rootSignature = app->getD3D12()->createRootSignature(rootSignatureDesc);

    return rootSignature != nullptr;
}

bool ShadowMapPass::createPSO()
{
    // Implementation for creating pipeline state object
    auto dataVS = DX::ReadData(L"shadowMapVS.cso");
    auto dataPS = DX::ReadData(L"shadowMapPS.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = Mesh::getInputLayoutDesc();
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.VS = { dataVS.data(), dataVS.size() };
    psoDesc.PS = { dataPS.data(), dataPS.size() };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc = { 1, 0 };
    psoDesc.SampleMask = 0xffffffff;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    psoDesc.NumRenderTargets = 1; 
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32_FLOAT;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

bool ShadowMapPass::createShadowMapResources()
{
    // Implementation for creating shadow map resources
    depth        = app->getResources()->createDepthStencil(DXGI_FORMAT_D32_FLOAT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 1, 1.0, 0, true, "ShadowMap");
    moments      = app->getResources()->createRenderTarget(DXGI_FORMAT_R32G32_FLOAT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 1, Vector4::Zero, "ShadowMapMoments");
    momentsBlur0 = app->getResources()->createUnorderedAccessTexture2D(DXGI_FORMAT_R32G32_FLOAT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, "ShadowMapMomentsBlur0");
    momentsBlur1 = app->getResources()->createUnorderedAccessTexture2D(DXGI_FORMAT_R32G32_FLOAT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, "ShadowMapMomentsBlur1");

    ModuleTargetDescriptors* targetDescriptors = app->getTargetDescriptors();

    dsvDesc = targetDescriptors->createDSV(depth.Get());
    rtvDesc = targetDescriptors->createRTV(moments.Get());

    srvDesc = app->getShaderDescriptors()->allocTable();
    srvDesc.createTextureSRV(moments.Get(), 0);
    srvDesc.createTextureSRV(momentsBlur0.Get(), 1);
    srvDesc.createTextureUAV(momentsBlur0.Get(), 2);
    srvDesc.createTextureUAV(momentsBlur1.Get(), 3);
    srvDesc.createTextureSRV(momentsBlur1.Get(), 4);

    return depth != nullptr && moments != nullptr && momentsBlur0 != nullptr && momentsBlur1 != nullptr;
}

bool ShadowMapPass::createBlurRootSignature()
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

bool ShadowMapPass::createBlurPSO()
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    auto dataCS = DX::ReadData(L"momentsBlurCS.cso");

    psoDesc.pRootSignature = blurRootSignature.Get();
    psoDesc.CS = { dataCS.data(), dataCS.size() };

    if (FAILED(app->getD3D12()->getDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&blurPSO))))
    {
        return false;
    }

    return true;
}
