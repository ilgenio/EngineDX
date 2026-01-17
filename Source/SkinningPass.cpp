#include "Globals.h"

#include "SkinningPass.h"

#include "Application.h"
#include "ModuleRingBuffer.h"
#include "ModuleD3D12.h"

#include "Scene.h"
#include "Mesh.h"

#include "ReadData.h"

SkinningPass::SkinningPass()
{
    createRootSignature();
    createPSO();
}

SkinningPass::~SkinningPass()
{

}

void SkinningPass::record(ID3D12GraphicsCommandList* commandList, std::span<RenderMesh> meshes)
{
    if (!meshes.empty())
    {
        BEGIN_EVENT(commandList, "Skinning Pass");

        ModuleRingBuffer* ringBuffer = app->getRingBuffer();

        commandList->SetComputeRootSignature(rootSignature.Get());
        commandList->SetPipelineState(pso.Get());

        for (auto& mesh : meshes)
        {
            if (mesh.numJoints > 0)
            {
                mesh.skinnedResult = ringBuffer->allocDefaultBuffer(mesh.mesh->getNumVertices() * sizeof(Mesh::Vertex));

                commandList->SetComputeRoot32BitConstant(0, mesh.mesh->getNumVertices(), 0);
                commandList->SetComputeRootShaderResourceView(1, app->getRingBuffer()->allocUploadBuffer(mesh.palette, sizeof(Matrix) * mesh.numJoints));
                commandList->SetComputeRootShaderResourceView(2, mesh.mesh->getVertexBuffer());
                commandList->SetComputeRootShaderResourceView(3, mesh.mesh->getBoneData());
                commandList->SetComputeRootUnorderedAccessView(4, mesh.skinnedResult);

                commandList->Dispatch(getDivisedSize(mesh.mesh->getNumVertices(), 64), 1, 1);
            }
        }

        END_EVENT(commandList);
    }
}

bool SkinningPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[5] = {};

    rootParameters[0].InitAsConstants((sizeof(INT32) / sizeof(UINT32)), 0);
    rootParameters[1].InitAsShaderResourceView(0);
    rootParameters[2].InitAsShaderResourceView(1);
    rootParameters[3].InitAsShaderResourceView(2);
    rootParameters[4].InitAsUnorderedAccessView(0);

    rootSignatureDesc.Init(5, rootParameters);  

    ComPtr<ID3DBlob> rootSignatureBlob;

    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, nullptr)))
    {
        return false;
    }

    if (FAILED(app->getD3D12()->getDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature))))
    {
        return false;
    }

    return true;
}

bool SkinningPass::createPSO()
{
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    auto dataCS = DX::ReadData(L"skinningCS.cso");
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.CS = { dataCS.data(), dataCS.size() };

    if (FAILED(app->getD3D12()->getDevice()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso))))
    {
        return false;
    }

    return true;
}