#include "Globals.h"

#include "SkinningPass.h"

#include "Application.h"
#include "ModuleD3D12.h"

#include "Scene.h"
#include "Mesh.h"

#include "ReadData.h"

#define MAX_NUM_OBJECTS 128
#define MAX_NUM_JOINTS  128
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

UINT SkinningPass::copyPalettes(ID3D12GraphicsCommandList* commandList, std::span<RenderMesh> meshes)
{
    _ASSERTE(meshes.empty() == false);

    ModuleD3D12* d3d12             = app->getD3D12();
    UINT currentBackBufferIndex    = d3d12->getCurrentBackBufferIdx();
    ComPtr<ID3D12Resource> palette = palettes[currentBackBufferIndex];

    UINT bytesCopied = 0;

    UINT8* mappedData = nullptr;
    CD3DX12_RANGE readRange(0, 0);
    upload->Map(0, &readRange, reinterpret_cast<void**>(&mappedData));

    for (auto& mesh : meshes)
    {
        if (mesh.numJoints > 0)
        {
            // Copy palette data to GPU
            memcpy(&mappedData[bytesCopied], mesh.palette, mesh.numJoints * sizeof(Matrix));
            bytesCopied += mesh.numJoints * sizeof(Matrix);
        }
    }

    upload->Unmap(0, nullptr);

    if(bytesCopied > 0)
    {
        // Copy to default buffer
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(palette.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->ResourceBarrier(1, &barrier);

        commandList->CopyBufferRegion(palettes[currentBackBufferIndex].Get(), 0, upload.Get(), 0, bytesCopied);

        barrier = CD3DX12_RESOURCE_BARRIER::Transition(palette.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &barrier);
    }

    return bytesCopied;
}

void SkinningPass::record(ID3D12GraphicsCommandList* commandList, std::span<RenderMesh> meshes)
{
    if (!meshes.empty())
    {
        BEGIN_EVENT(commandList, "Skinning Pass");

        if(copyPalettes(commandList, meshes) > 0)
        {
            ModuleD3D12* d3d12 = app->getD3D12();
            UINT currentBackBufferIndex = d3d12->getCurrentBackBufferIdx();
            ComPtr<ID3D12Resource> output = outputs[currentBackBufferIndex];
            ComPtr<ID3D12Resource> palette = palettes[currentBackBufferIndex];

            commandList->SetComputeRootSignature(rootSignature.Get());
            commandList->SetPipelineState(pso.Get());

            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(output.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            commandList->ResourceBarrier(1, &barrier);

            UINT paletteOffset = 0;
            UINT outputOffset = 0;

            for (auto& mesh : meshes)
            {
                if (mesh.numJoints > 0)
                {
                    commandList->SetComputeRoot32BitConstant(0, mesh.mesh->getNumVertices(), 0);
                    commandList->SetComputeRootShaderResourceView(1, palette->GetGPUVirtualAddress()+paletteOffset);
                    commandList->SetComputeRootShaderResourceView(2, mesh.mesh->getVertexBuffer());
                    commandList->SetComputeRootShaderResourceView(3, mesh.mesh->getBoneData());
                    commandList->SetComputeRootUnorderedAccessView(4, output->GetGPUVirtualAddress()+outputOffset);

                    commandList->Dispatch(getDivisedSize(mesh.mesh->getNumVertices(), 64), 1, 1);

                    paletteOffset += mesh.numJoints * sizeof(Matrix);
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

bool SkinningPass::buildBuffers()
{
    auto* device = app->getD3D12()->getDevice();

    CD3DX12_HEAP_PROPERTIES defaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    for (UINT i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        CD3DX12_RESOURCE_DESC outputDesc = CD3DX12_RESOURCE_DESC::Buffer(MAX_NUM_OBJECTS* MAX_NUM_VERTICES * sizeof(Mesh::Vertex), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        device->CreateCommittedResource(&defaultProps, D3D12_HEAP_FLAG_NONE, &outputDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&outputs[i]));
        outputs[i]->SetName(L"Skinning Output Buffer");

        CD3DX12_RESOURCE_DESC paletteDesc  = CD3DX12_RESOURCE_DESC::Buffer(MAX_NUM_OBJECTS * MAX_NUM_JOINTS * sizeof(Matrix));
        device->CreateCommittedResource(&defaultProps, D3D12_HEAP_FLAG_NONE, &paletteDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&palettes[i]));
        palettes[i]->SetName(L"Skinning Palette Buffer");

    }


    CD3DX12_HEAP_PROPERTIES uploadProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(MAX_NUM_OBJECTS * MAX_NUM_JOINTS * sizeof(Matrix));
    device->CreateCommittedResource(&uploadProps, D3D12_HEAP_FLAG_NONE, &uploadDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&upload));
    upload->SetName(L"Skinning Upload Buffer");


    return true;
}

