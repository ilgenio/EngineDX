#include "Globals.h"

#include "SkinningPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ModuleRingBuffer.h"

#include "Scene.h"
#include "Mesh.h"

#include "ReadData.h"

#define MAX_NUM_OBJECTS 128
#define MAX_NUM_VERTICES 64 * (1 << 10)

SkinningPass::SkinningPass()
{
    createRootSignature();
    createPSO();
}

SkinningPass::~SkinningPass()
{

}

void SkinningPass::copyPalette(const RenderMesh& mesh, std::vector<Matrix>& palette)
{
    if (mesh.numJoints > 0)
    {
        palette.reserve(mesh.numJoints * 2);

        // For positions
        for (UINT i = 0; i < mesh.numJoints; ++i)
        {
            palette.push_back(mesh.palette[i].Transpose());
        }

        // For normals
        for (UINT i = 0; i < mesh.numJoints; ++i)
        {
            palette.push_back(mesh.palette[i].Invert());
        }
    }

}

void SkinningPass::copyMorphWeights(const RenderMesh& mesh, std::vector<float>& morphWeights)
{
    if (mesh.numMorphTargets > 0)
    {
        morphWeights.reserve(mesh.numMorphTargets);

        for (UINT i = 0; i < mesh.numMorphTargets; ++i)
        {
            morphWeights.push_back(mesh.morphWeights[i]);
        }
    }
}


void SkinningPass::render(ID3D12GraphicsCommandList* commandList, std::span<RenderMesh> meshes)
{
    if (!meshes.empty())
    {
        BEGIN_EVENT(commandList, "Skinning Pass");


        std::vector<Matrix> palette;
        std::vector<float> morphWeights;

        ModuleD3D12* d3d12 = app->getD3D12();
        ModuleRingBuffer* ringBuffer = app->getRingBuffer();

        commandList->SetComputeRootSignature(rootSignature.Get());
        commandList->SetPipelineState(pso.Get());

        for (auto& mesh : meshes)
        {
            if (mesh.numJoints > 0 || mesh.numMorphTargets > 0)
            {
                palette.clear();
                morphWeights.clear();

                copyPalette(mesh, palette);
                copyMorphWeights(mesh, morphWeights);

                D3D12_GPU_VIRTUAL_ADDRESS paletteAddress = ringBuffer->alloc(palette.data(), palette.size());
                D3D12_GPU_VIRTUAL_ADDRESS weightsAddress = ringBuffer->alloc(morphWeights.data(), morphWeights.size());

                mesh.mesh->swapDeformationBuffer();
                ID3D12Resource* output = mesh.mesh->getCurrentDeformationBuffer();

                auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(output, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
                commandList->ResourceBarrier(1, &barrier);

                commandList->SetComputeRoot32BitConstant(ROOTPARAM_NUM_VERTICES, mesh.mesh->getNumVertices(), 0);
                commandList->SetComputeRoot32BitConstant(ROOTPARAM_NUM_VERTICES, mesh.numJoints, 1);
                commandList->SetComputeRoot32BitConstant(ROOTPARAM_NUM_VERTICES, mesh.numMorphTargets, 2);
                commandList->SetComputeRootShaderResourceView(ROOTPARAM_PALETTE, paletteAddress);
                commandList->SetComputeRootShaderResourceView(ROOTPARAM_PALETTE_NORMAL, paletteAddress + mesh.numJoints * sizeof(Matrix));
                commandList->SetComputeRootShaderResourceView(ROOTPARAM_VERTICES, mesh.mesh->getVertexBuffer());
                commandList->SetComputeRootShaderResourceView(ROOTPARAM_BONE_WEIGHTS, mesh.mesh->getBoneData());
                commandList->SetComputeRootUnorderedAccessView(ROOTPARAM_OUTPUT, output->GetGPUVirtualAddress());
                commandList->SetComputeRootShaderResourceView(ROOTPARAM_MORPH_WEIGHTS, weightsAddress);
                commandList->SetComputeRootShaderResourceView(ROOTPARAM_MORPH_VERTICES, mesh.mesh->getMorphBuffer());
                commandList->Dispatch(getDivisedSize(mesh.mesh->getNumVertices(), 64), 1, 1);

                barrier = CD3DX12_RESOURCE_BARRIER::Transition(output, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                commandList->ResourceBarrier(1, &barrier);
            }
        }

        END_EVENT(commandList);
    }
}

bool SkinningPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[ROOTPARAM_COUNT] = {};

    rootParameters[ROOTPARAM_NUM_VERTICES].InitAsConstants((sizeof(INT32) * 3 / sizeof(UINT32)), 0);
    rootParameters[ROOTPARAM_PALETTE].InitAsShaderResourceView(0);
    rootParameters[ROOTPARAM_PALETTE_NORMAL].InitAsShaderResourceView(1);
    rootParameters[ROOTPARAM_VERTICES].InitAsShaderResourceView(2);
    rootParameters[ROOTPARAM_BONE_WEIGHTS].InitAsShaderResourceView(3);
    rootParameters[ROOTPARAM_MORPH_WEIGHTS].InitAsShaderResourceView(4);
    rootParameters[ROOTPARAM_MORPH_VERTICES].InitAsShaderResourceView(5);
    rootParameters[ROOTPARAM_OUTPUT].InitAsUnorderedAccessView(0);

    rootSignatureDesc.Init(ROOTPARAM_COUNT, rootParameters);  

    rootSignature = app->getD3D12()->createRootSignature(rootSignatureDesc);

    return rootSignature != nullptr;
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

