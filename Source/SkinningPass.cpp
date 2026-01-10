#include "Globals.h"

#include "SkinningPass.h"
#include "Scene.h"
#include "ModuleRingBuffer.h"

SkinningPass::SkinningPass()
{

}

SkinningPass::~SkinningPass()
{

}

void SkinningPass::record(ID3D12GraphicsCommandList* commandList, std::span<RenderMesh> meshes)
{

}

D3D12_GPU_VIRTUAL_ADDRESS SkinningPass::buildMatrixPalette(const RenderMesh& mesh)
{
    std::vector<Matrix> boneMatrices(mesh.mesh->getNumBones());

#if 0
    for (UINT i = 0; i < mesh.mesh->getNumBones(); ++i)
    {
        UINT parentIndex = mesh.mesh->getBoneParentIndex(i);
        Matrix localTransform = mesh.mesh->getBoneLocalTransform(i);

        if (parentIndex == UINT_MAX)
        {
            boneMatrices[i] = localTransform;
        }
        else
        {
            boneMatrices[i] = localTransform * boneMatrices[parentIndex];
        }

        boneMatrices[i] = boneMatrices[i] * mesh.mesh->getBoneOffsetMatrix(i);
    }
#endif 

    return app->getRingBufferModule()->allocBuffer(boneMatrices.data(), boneMatrices.size());
}

bool SkinningPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
    CD3DX12_DESCRIPTOR_RANGE tableRanges;

    tableRanges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);

    rootParameters[0].InitAsConstants((sizeof(INT32) / sizeof(UINT32)), 0);
    rootParameters[1].InitAsDescriptorTable(1, &tableRanges);
    rootParameters[2].InitAsDescriptorTable(1, &tableRanges);

    rootSignatureDesc.Init(3, rootParameters);

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
    return true;
}