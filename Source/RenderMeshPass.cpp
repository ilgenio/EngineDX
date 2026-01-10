#include "Globals.h"

#include "RenderMeshPass.h"

#include "Scene.h"
#include "Mesh.h"

#include "Application.h"
#include "ModuleSamplers.h"
#include "ModuleD3D12.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleResources.h"
#include "ModuleCamera.h"
#include "ModuleRingBuffer.h"

#include "ReadData.h"

RenderMeshPass::RenderMeshPass()
{

}

RenderMeshPass::~RenderMeshPass()
{

}

bool RenderMeshPass::init(bool useMSAA)
{

    bool ok = createRootSignature();
    ok  = ok && createPSO(useMSAA);

    _ASSERT_EXPR(ok, "Error creating RenderMeshPass");

    return ok;
}

void RenderMeshPass::render(ID3D12GraphicsCommandList* commandList, std::span<const RenderMesh> meshes, D3D12_GPU_VIRTUAL_ADDRESS perFrameData, D3D12_GPU_DESCRIPTOR_HANDLE iblTable, const Matrix& viewProjection)
{
    BEGIN_EVENT(commandList, "RenderMesh Pass");

    ModuleRingBuffer* ringBuffer = app->getRingBuffer();
    ModuleSamplers* samplers = app->getSamplers();

    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pso.Get());

    commandList->SetGraphicsRootConstantBufferView(SLOT_PER_FRAME_CB, perFrameData);
    commandList->SetGraphicsRootDescriptorTable(SLOT_IBL_TABLE, iblTable);
    commandList->SetGraphicsRootDescriptorTable(SLOT_SAMPLERS, samplers->getGPUHandle(ModuleSamplers::LINEAR_WRAP));

    for (const RenderMesh& mesh : meshes)
    {
        if( mesh.material )
        {
            Matrix mvp = (mesh.transform * viewProjection).Transpose();
            commandList->SetGraphicsRoot32BitConstants(SLOT_MVP_MATRIX, sizeof(Matrix) / sizeof(UINT32), &mvp, 0);

            PerInstance perInstance;
            perInstance.modelMat = mesh.transform.Transpose();
            perInstance.normalMat = mesh.normalMatrix.Transpose();
            perInstance.material = mesh.material->getData();

            commandList->SetGraphicsRootConstantBufferView(SLOT_PER_INSTANCE_CB, ringBuffer->allocUploadBuffer(&perInstance));
            commandList->SetGraphicsRootDescriptorTable(SLOT_TEXTURES_TABLE, mesh.material->getTextureTable());
            mesh.mesh->draw(commandList);
        }
    }

    END_EVENT(commandList);
}

bool RenderMeshPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[SLOT_COUNT] = {};
    CD3DX12_DESCRIPTOR_RANGE lightsTableRange, iblTableRange, materialTableRange;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    lightsTableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);
    iblTableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 3);
    materialTableRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, Material::getNumTextureSlots(), 6);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);

    rootParameters[SLOT_MVP_MATRIX].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[SLOT_PER_FRAME_CB].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[SLOT_PER_INSTANCE_CB].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[SLOT_LIGHTS_TABLE].InitAsDescriptorTable(1, &lightsTableRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[SLOT_IBL_TABLE].InitAsDescriptorTable(1, &iblTableRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[SLOT_TEXTURES_TABLE].InitAsDescriptorTable(1, &materialTableRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[SLOT_SAMPLERS].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(SLOT_COUNT, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob)))
    {
        std::wstring msg((char*)errorBlob->GetBufferPointer(), (char*)errorBlob->GetBufferPointer()+errorBlob->GetBufferSize());
        _ASSERT_EXPR(false, msg.c_str());

        return false;
    }

    if (FAILED(app->getD3D12()->getDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature))))
    {
        return false;
    }

    return true;

}

bool RenderMeshPass::createPSO(bool useMSAA)
{
    auto dataVS = DX::ReadData(L"forwardVS.cso");
    auto dataPS = DX::ReadData(L"forwardPS.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout =  Mesh::getInputLayoutDesc();                                              // the structure describing our input layout
    psoDesc.pRootSignature = rootSignature.Get();                                                   // the root signature that describes the input data this pso needs
    psoDesc.VS = { dataVS.data(), dataVS.size() };                                                  // structure describing where to find the vertex shader bytecode and how large it is
    psoDesc.PS = { dataPS.data(), dataPS.size() };                                                  // same as VS but for pixel shader
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;                         // type of topology we are drawing
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                                             // format of the render target
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc = { useMSAA ? UINT(4) : UINT(1), 0};                                                                    // must be the same sample description as the swapchain and depth/stencil buffer
    psoDesc.SampleMask = 0xffffffff;                                                                // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);                               // a default rasterizer state.
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;                                           // our models are counter clock wise
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         // a default blend state.
    psoDesc.NumRenderTargets = 1;                                                                   // we are only binding one render target

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}
