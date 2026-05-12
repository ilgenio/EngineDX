#include "Globals.h"

#include "ShadowMapPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ModuleTargetDescriptors.h"
#include "ModuleShaderDescriptors.h"

#include "RenderStructs.h"
#include "Scene.h"

#include "ReadData.h"

#include "Mesh.h"

#define SHADOW_MAP_SIZE 1 << 13

ShadowMapPass::ShadowMapPass()
{
    createRootSignature();
    createPSO();
    createShadowMapResource();
}

ShadowMapPass::~ShadowMapPass()
{

}

void ShadowMapPass::render(ID3D12GraphicsCommandList* commandList, std::span<const RenderMesh> meshes, const RenderData& renderData)
{
    BEGIN_EVENT(commandList, "ShadowMap Pass");

    ModuleRingBuffer* ringBuffer = app->getRingBuffer();
    ModuleSamplers* samplers = app->getSamplers();

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pso.Get());

    D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsvDesc.getCPUHandle();
    

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(shadowMap.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    commandList->ResourceBarrier(1, &barrier);

    commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
    commandList->ClearDepthStencilView(dsvDesc.getCPUHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

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

        commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &model, 0);
        commandList->SetGraphicsRootConstantBufferView(1, renderData.shadowViewProjBuffer);

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

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(shadowMap.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &barrier);

    END_EVENT(commandList);
}

bool ShadowMapPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameter[2] = {};

    rootParameter[0].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameter[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    rootSignatureDesc.Init(2, rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

    psoDesc.NumRenderTargets = 0; // No render targets, only depth
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

bool ShadowMapPass::createShadowMapResource()
{
    // Implementation for creating pipeline state object
    shadowMap = app->getResources()->createDepthStencil(DXGI_FORMAT_D32_FLOAT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 1, 1.0, 0, false, "ShadowMap");

    ModuleTargetDescriptors* targetDescriptors = app->getTargetDescriptors();

    dsvDesc = targetDescriptors->createDS(shadowMap.Get());

    srvDesc = app->getShaderDescriptors()->allocTable();
    srvDesc.createTexture2DSRV(shadowMap.Get(), DXGI_FORMAT_R32_FLOAT);

    return shadowMap != nullptr;
}

