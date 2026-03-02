#include "Globals.h"

#include "GBufferExportPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleSamplers.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleRingBuffer.h"

#include "Scene.h"
#include "Mesh.h"

#include "ReadData.h"

GBufferExportPass::GBufferExportPass()
{
}

GBufferExportPass::~GBufferExportPass()
{
}

void GBufferExportPass::resize(UINT sizeX, UINT sizeY)
{
    gBuffer.resize(sizeX, sizeY);
}

bool GBufferExportPass::init()
{
    bool ok = createRootSignature();
    ok = ok && createPSO();

    return ok;
}

void GBufferExportPass::render(ID3D12GraphicsCommandList* commandList, std::span<const RenderMesh> meshes, D3D12_GPU_VIRTUAL_ADDRESS skinningBuffer, const Matrix& viewProjection)
{
    BEGIN_EVENT(commandList, "GBufferExport Pass");

    ModuleRingBuffer* ringBuffer = app->getRingBuffer();
    ModuleSamplers* samplers = app->getSamplers();

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pso.Get());

    gBuffer.beginRender(commandList);

    commandList->SetGraphicsRootDescriptorTable(SLOT_SAMPLERS, samplers->getGPUHandle(ModuleSamplers::LINEAR_WRAP));
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (const RenderMesh& mesh : meshes)
    {
        if (mesh.material)
        {
            Matrix mvp;

            PerInstance perInstance;

            if (mesh.numJoints > 0) // skinned mesh
            {
                // Note: Skinned mesh has already bee transformed in the skinning pass 
                mvp = (viewProjection).Transpose();
                perInstance.modelMat = Matrix::Identity;
                perInstance.normalMat = Matrix::Identity;

                D3D12_VERTEX_BUFFER_VIEW vertexBufferView = mesh.mesh->getVertexBufferView();
                vertexBufferView.BufferLocation = skinningBuffer + mesh.skinningOffset;

                commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
            }
            else // rigid mesh
            {
                mvp = (mesh.transform * viewProjection).Transpose();
                perInstance.modelMat = mesh.transform.Transpose();
                perInstance.normalMat = mesh.normalMatrix.Transpose();

                commandList->IASetVertexBuffers(0, 1, &mesh.mesh->getVertexBufferView());
            }

            perInstance.material = mesh.material->getData();

            commandList->SetGraphicsRoot32BitConstants(SLOT_MVP_MATRIX, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);
            commandList->SetGraphicsRootConstantBufferView(SLOT_PER_INSTANCE_CB, ringBuffer->alloc(&perInstance));
            commandList->SetGraphicsRootDescriptorTable(SLOT_TEXTURES_TABLE, mesh.material->getTextureTable());

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
    }

    gBuffer.endRender(commandList);

    END_EVENT(commandList);
}

bool GBufferExportPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[SLOT_COUNT] = {};
    CD3DX12_DESCRIPTOR_RANGE materialTableRange;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    materialTableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Material::getNumTextureSlots(), 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);

    rootParameters[SLOT_MVP_MATRIX].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[SLOT_PER_INSTANCE_CB].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[SLOT_TEXTURES_TABLE].InitAsDescriptorTable(1, &materialTableRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[SLOT_SAMPLERS].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(SLOT_COUNT, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    rootSignature = app->getD3D12()->createRootSignature(rootSignatureDesc);

    return rootSignature;
}

bool GBufferExportPass::createPSO()
{
    // Implementation for creating pipeline state object
    auto dataVS = DX::ReadData(L"gbufferVS.cso");
    auto dataPS = DX::ReadData(L"gbufferPS.cso");

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

    // Set render target formats
    for(UINT i=0; i<GBuffer::getRTFormatCount(); ++i)
    {
        psoDesc.RTVFormats[i] = GBuffer::getRTFormat(i);
    }

    psoDesc.DSVFormat = GBuffer::getDepthFormat();
    psoDesc.NumRenderTargets = GBuffer::getRTFormatCount(); 

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

