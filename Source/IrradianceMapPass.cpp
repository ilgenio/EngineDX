#include "Globals.h"
#include "IrradianceMapPass.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "ModuleResources.h"
#include "ModuleRTDescriptors.h"
#include "ModuleShaderDescriptors.h"
#include "ModuleSamplers.h"
#include "CubemapMesh.h"
#include "ReadData.h"
#include "Math.h"

IrradianceMapPass::IrradianceMapPass()
{
    createRootSignature();
    createPSO();
    cubemapMesh = std::make_unique<CubemapMesh>();
}

IrradianceMapPass::~IrradianceMapPass()
{
}

void IrradianceMapPass::record(ID3D12GraphicsCommandList* cmdList, UINT cubeMapDesc, uint32_t width, uint32_t height)
{
    ModuleResources* resources = app->getResources();
    ModuleShaderDescriptors* descriptors = app->getShaderDescriptors();
    ModuleSamplers* samplers = app->getSamplers();

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    irradianceMap = resources->createCubemapRenderTarget(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height, clearColor, "Irradiance Map");

    // set necessary state
    cmdList->SetPipelineState(pso.Get());
    cmdList->SetGraphicsRootSignature(rootSignature.Get());

    // set viewport and scissor

    D3D12_VIEWPORT viewport;
    viewport.TopLeftX = viewport.TopLeftY = 0;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.Width = float(width);
    viewport.Height = float(height);

    D3D12_RECT scissor;
    scissor.left = 0;
    scissor.top = 0;
    scissor.right = width;
    scissor.bottom = height;

    cmdList->RSSetViewports(1, &viewport);
    cmdList->RSSetScissorRects(1, &scissor);

    cmdList->IASetVertexBuffers(0, 1, &cubemapMesh->getVertexBufferView());

    cmdList->SetGraphicsRootDescriptorTable(1, descriptors->getGPUHandle(cubeMapDesc));
    cmdList->SetGraphicsRootDescriptorTable(2, samplers->getGPUHandle(ModuleSamplers::LINEAR_WRAP));

    // create render target view for each face
    ModuleRTDescriptors* rtDescriptors = app->getRTDescriptors();
    Matrix projMatrix = Matrix::CreatePerspectiveFieldOfView(HALF_PI, 1.0f, 0.1f, 100.0f);

    for(int i=0; i<6; ++i)
    {
        Matrix viewMatrix = Matrix::CreateLookAt(Vector3(0.0f), cubemapMesh->getFrontDir(CubemapMesh::Direction(i)), cubemapMesh->getUpDir(CubemapMesh::Direction(i)));
        Matrix mvpMatrix = (viewMatrix * projMatrix).Transpose();

        cmdList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvpMatrix, 0);

        //CD3DX12_RESOURCE_BARRIER toRT = CD3DX12_RESOURCE_BARRIER::Transition(irradianceMap.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET, i);
        //cmdList->ResourceBarrier(1, &toRT);

        UINT rtvHandle = rtDescriptors->create(irradianceMap.Get(), i, DXGI_FORMAT_R16G16B16A16_FLOAT);
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = rtDescriptors->getCPUHandle(rtvHandle);
        cmdList->OMSetRenderTargets(1, &cpuHandle, FALSE, nullptr);

        cmdList->DrawInstanced(cubemapMesh->getVertexCount(), 1, 0, 0);

        CD3DX12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(irradianceMap.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, i);
        cmdList->ResourceBarrier(1, &toSRV);
    }
}

bool IrradianceMapPass::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
    CD3DX12_DESCRIPTOR_RANGE tableRanges;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    tableRanges.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, ModuleSamplers::COUNT, 0);

    rootParameters[0].InitAsConstants((sizeof(Matrix) / sizeof(UINT32)), 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsDescriptorTable(1, &tableRanges, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    rootSignatureDesc.Init(3, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

bool IrradianceMapPass::createPSO()
{
    auto dataVS = DX::ReadData(L"fullscreenVS.cso");
    auto dataPS = DX::ReadData(L"IrradianceMapPS.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = cubemapMesh->getInputLayoutDesc();
    psoDesc.pRootSignature = rootSignature.Get();                                                   
    psoDesc.VS = { dataVS.data(), dataVS.size() };                                                  
    psoDesc.PS = { dataPS.data(), dataPS.size() };                                                  
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;                         
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;                                         
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc = {1, 0};                                                                    
    psoDesc.SampleMask = 0xffffffff;                                                                
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);                               
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE; 
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         
    psoDesc.NumRenderTargets = 1;                                                                   

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}