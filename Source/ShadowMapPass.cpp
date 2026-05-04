#include "Globals.h"

#include "ShadowMapPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"

#include "RenderStructs.h"
#include "Scene.h"

#include "ReadData.h"

#include "Mesh.h"

#define SHADOW_MAP_SIZE 1024

ShadowMapPass::ShadowMapPass()
{
    createRootSignature();
    createPSO();
    createShadowMapResource();
}

ShadowMapPass::~ShadowMapPass()
{

}

void ShadowMapPass::buildFrustum(Vector4 planes[6], const Vector3& lightDir, const Vector4& sphereBound)
{
    float sphereRadius = sphereBound.w;
    Vector3 sphereCenter = Vector3(sphereBound.x, sphereBound.y, sphereBound.z);

    Matrix proj = Matrix::CreateOrthographic(sphereRadius * 2.0f, sphereRadius * 2.0f, 0.0f, sphereRadius * 2.0f);

    Vector3 eye = sphereCenter - lightDir * sphereRadius;
    Vector3 up = Vector3::Up;
    Matrix view = Matrix::CreateLookAt(eye, sphereCenter, up);

    viewProj = view * proj;

    getPlanes(planes, viewProj, false);
}

void ShadowMapPass::render(ID3D12GraphicsCommandList* commandList, std::span<const RenderMesh> meshes, const RenderData& renderData)
{
    BEGIN_EVENT(commandList, "ShadowMap Pass");

    ModuleRingBuffer* ringBuffer = app->getRingBuffer();
    ModuleSamplers* samplers = app->getSamplers();

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pso.Get());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (const RenderMesh& mesh : meshes)
    {
        Matrix mvp;

        if (mesh.numJoints > 0 || mesh.numMorphTargets > 0) // skinned mesh
        {
            // Note: Skinned mesh has already bee transformed in the skinning pass 
            mvp = viewProj.Transpose();

            D3D12_VERTEX_BUFFER_VIEW vertexBufferView = mesh.mesh->getVertexBufferView();
            vertexBufferView.BufferLocation = renderData.skinningBuffer + mesh.skinningOffset;

            commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
        }
        else // rigid mesh
        {
            mvp = (mesh.transform * viewProj).Transpose();

            commandList->IASetVertexBuffers(0, 1, &mesh.mesh->getVertexBufferView());
        }

        commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);

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

bool ShadowMapPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameter = {};

    rootParameter.InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    rootSignatureDesc.Init(1, &rootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    rootSignature = app->getD3D12()->createRootSignature(rootSignatureDesc);

    return rootSignature != nullptr;
}

bool ShadowMapPass::createPSO()
{
    // Implementation for creating pipeline state object
    shadowMap = app->getResources()->createDepthStencil(DXGI_FORMAT_D32_FLOAT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 1, 1.0, 1, "ShadowMap");

    return shadowMap != nullptr;
}

bool ShadowMapPass::createShadowMapResource()
{
    // Implementation for creating pipeline state object
    auto dataVS = DX::ReadData(L"shadowMapVS.cso");
    auto dataPS = DX::ReadData(L"shadowMapPS.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout =  Mesh::getInputLayoutDesc();                       
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

    psoDesc.NumRenderTargets = 0; // No render targets, only depth
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso))); 
}