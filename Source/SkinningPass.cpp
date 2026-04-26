#include "Globals.h"

#include "SkinningPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleDynamicBuffer.h"
#include "ModuleResources.h"

#include "Scene.h"
#include "Mesh.h"

#include "ReadData.h"

#define MAX_NUM_OBJECTS 128
#define MAX_NUM_VERTICES 64 * (1 << 10)

SkinningPass::SkinningPass()
{
    createRootSignature();
    createPSO();
    buildBuffers();
}

SkinningPass::~SkinningPass()
{

}

std::vector<Matrix> SkinningPass::copyPalettes(ID3D12GraphicsCommandList* commandList, std::span<RenderMesh> meshes)
{
    _ASSERTE(meshes.empty() == false);

    std::vector<Matrix> palette;

    for (auto& mesh : meshes)
    {
        if (mesh.numJoints > 0)
        {
            // For positions
            for(UINT i=0; i<mesh.numJoints; ++i)
            {
                palette.push_back(mesh.palette[i].Transpose());
            }

            // For normals
            for(UINT i=0; i<mesh.numJoints; ++i)
            {
                palette.push_back(mesh.palette[i].Invert());
            }
        }
    }

    return palette;
}

std::vector<float> SkinningPass::copyMorphWeights(ID3D12GraphicsCommandList* commandList, std::span<RenderMesh> meshes)
{
    _ASSERTE(meshes.empty() == false);

    std::vector<float> morphWeights;

    for (auto& mesh : meshes)
    {
        if (mesh.numMorphTargets > 0)
        {
            for (UINT i = 0; i < mesh.numMorphTargets; ++i)
            {
                morphWeights.push_back(mesh.morphWeights[i]);
            }
        }
    }

    return morphWeights;
}


void SkinningPass::record(ID3D12GraphicsCommandList* commandList, std::span<RenderMesh> meshes)
{
    if (!meshes.empty())
    {
        BEGIN_EVENT(commandList, "Skinning Pass");

        std::vector<Matrix> palettes = copyPalettes(commandList, meshes);
        std::vector<float> morphWeights = copyMorphWeights(commandList, meshes);

        if(!palettes.empty() || !morphWeights.empty())
        {
            ModuleD3D12* d3d12 = app->getD3D12();
            ModuleDynamicBuffer* dynamicBuffer = app->getDynamicBuffer();

            D3D12_GPU_VIRTUAL_ADDRESS palettesAddress = dynamicBuffer->alloc(palettes.data(), palettes.size());
            D3D12_GPU_VIRTUAL_ADDRESS weightsAddress = dynamicBuffer->alloc(morphWeights.data(), morphWeights.size());

            dynamicBuffer->submitCopy(commandList);

            commandList->SetComputeRootSignature(rootSignature.Get());
            commandList->SetPipelineState(pso.Get());

            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(output.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            commandList->ResourceBarrier(1, &barrier);

            UINT paletteOffset = 0;
            UINT weightsOffset = 0;
            UINT outputOffset = 0;

            for (auto& mesh : meshes)
            {
                if (mesh.numJoints > 0 || mesh.numMorphTargets > 0)
                {
                    UINT paletteBytes = mesh.numJoints * sizeof(Matrix);
                    UINT morphBytes = mesh.numMorphTargets * sizeof(float);

                    commandList->SetComputeRoot32BitConstant(ROOTPARAM_NUM_VERTICES, mesh.mesh->getNumVertices(), 0);
                    commandList->SetComputeRoot32BitConstant(ROOTPARAM_NUM_VERTICES, mesh.numJoints, 1);
                    commandList->SetComputeRoot32BitConstant(ROOTPARAM_NUM_VERTICES, mesh.numMorphTargets, 2);
                    commandList->SetComputeRootShaderResourceView(ROOTPARAM_PALETTE, palettesAddress + paletteOffset);
                    commandList->SetComputeRootShaderResourceView(ROOTPARAM_PALETTE_NORMAL, palettesAddress + paletteOffset + paletteBytes);
                    commandList->SetComputeRootShaderResourceView(ROOTPARAM_VERTICES, mesh.mesh->getVertexBuffer());
                    commandList->SetComputeRootShaderResourceView(ROOTPARAM_BONE_WEIGHTS, mesh.mesh->getBoneData());
                    commandList->SetComputeRootUnorderedAccessView(ROOTPARAM_OUTPUT, output->GetGPUVirtualAddress()+outputOffset);

                    commandList->SetComputeRootShaderResourceView(ROOTPARAM_MORPH_WEIGHTS, weightsAddress + weightsOffset);    
                    commandList->SetComputeRootShaderResourceView(ROOTPARAM_MORPH_VERTICES, mesh.mesh->getMorphBuffer());

                    commandList->Dispatch(getDivisedSize(mesh.mesh->getNumVertices(), 64), 1, 1);

                    mesh.skinningOffset = outputOffset; 
                    paletteOffset += paletteBytes*2;
                    weightsOffset += morphBytes;
                    outputOffset += mesh.mesh->getNumVertices() * sizeof(Mesh::Vertex);
                }
            }

            barrier = CD3DX12_RESOURCE_BARRIER::Transition(output.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            commandList->ResourceBarrier(1, &barrier);
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

bool SkinningPass::buildBuffers()
{
    ModuleResources* resources = app->getResources();

    output = resources->createUnorderedAccessBuffer(MAX_NUM_OBJECTS * MAX_NUM_VERTICES * sizeof(Mesh::Vertex), "Skinning Output Buffer");

    return true;
}

D3D12_GPU_VIRTUAL_ADDRESS SkinningPass::getOutputAddress() const
{
    return output->GetGPUVirtualAddress();
}
